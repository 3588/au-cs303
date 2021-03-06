/*
 * This source file is derived from Maq v0.6.6.  It is distributed
 * under the GNU GENERAL PUBLIC LICENSE v2 with no warranty.  See file
 * "COPYING" in this directory for the terms.
 */

#include <string>
#include <iostream>
#include <set>
#include <map>
#include <stdio.h>
#include <inttypes.h>
#include <seqan/sequence.h>
#include <stdexcept>
#include <algorithm>
#include "maqmap.h"
#include "algo.hh"
#include "bfa.h"
#include "tokenize.h"
#include "formats.h"
#include "pat.h"

enum {TEXT_MAP, BIN_MAP};

using namespace std;
static bool verbose	= false;
static bool short_map_format = false;
static const int short_read_len = 64;
static const int long_read_len = 128;

// A lookup table for fast integer logs
static int log_n[256];

/**
 * Print a detailed usage message to the provided output stream.
 *
 * Manual text converted to C++ string with something like:
 * cat MANUAL  | tail -240 | sed -e 's/\"/\\\"/g' | \
 *   sed -e 's/^/"/' | sed -e 's/$/\\n"/'
 */
static void printLongUsage(ostream& out) {
	out <<
	"\n"
	" Using the 'bowtie-maqconvert' Alignment Converter\n"
	" -------------------------------------------------\n"
	"\n"
	" 'bowtie-maqconvert' allows the user to convert from Bowtie's default\n"
	" alignment format to the format used by Maq.  This is a necessary first\n"
	" step to using Maq's analyses to study Bowtie's output, including\n"
	" consensus ('maq assemble') and SNP calling ('maq cns2snp').\n"
	" \n"
	" The conversion includes an in-memory sort of all alignments with\n"
	" respect to their position on the reference.  This can be problematic\n"
	" for very large sets of alignments; sorting so many alignments may\n"
	" exhaust memory, causing bowtie-maqconvert to crash or run very slowly.\n"
	" One solution is to split alignments into batches and use\n"
	" 'maq mapmerge' to combine them.  Another solution, depending on your\n"
	" application and reference, may be to use bowtie's --refout option at\n"
	" alignment time to pre-partition alignments into separate files\n"
	" according to reference sequence hit.\n"
	"\n"
	" Note that Maq changed its output format as of version 0.7.0 in order\n"
	" to accomodate read lengths up to 127 bases.  Be sure to use the -o\n"
	" option of bowtie-maqconvert if you plan to use the results with a Maq\n"
	" version prior to 0.7.0.\n"
	"\n"
	"  Command Line\n"
	"  ------------\n"
	"\n"
	" Usage: bowtie-maqconvert [options]* <in.bwtmap> <out.map> <chr.bfa>\n"
	"\n"
	"    <in.bwtmap>   An alignments file generated by Bowtie.  Need not\n"
	"                  have any particular extension.\n"
	"\n"
	"    <out.map>     Name of Maq-alignment file to output.  Need not have\n"
	"                  any particular extension.\n"
	"\n"
	"    <chr.bfa>     The .bfa version of the reference sequences(s).  Use\n"
	"                  'maq fasta2bfa' to build this.  It must be built\n"
	"                  using the exact same reference sequences as were used\n"
	"                  to build the Bowtie index.\n"
	"\n"
	" Options:\n"
	"\n"
	"    -o            Output uses Maq's old (pre-0.7.0) format.\n"
	"\n"
	"    -v            Print verbose output (for debugging).\n"
	"\n"
	"    -h            Print detailed description of tool and its options\n"
	"                  (from MANUAL).\n"
	"\n"
	;
}

static void printUsage(ostream& out) {
	out <<
	"Usage: bowtie-convert [options]* <in.bwtmap> <out.map> <chr.bfa>\n"
	"    <in.bwtmap>   Alignments generated by Bowtie\n"
	"    <out.map>     Name of Maq-compatible alignment file to output\n"
	"    <chr.bfa>     .bfa file for reference sequences; must be built with same\n"
	"                  reference sequences used in Bowtie alignment, in same order\n"
	"Options:\n"
	"    -o            output Maq map in old (pre Maq 0.7.0) format\n"
	"    -v            verbose output\n"
	"    -h            print detailed description of tool and its options\n"
	"\n"
	;
}

// Some of these limits come from Maq headers, beware...
static const int max_read_name = MAX_NAMELEN;
//static const int max_read_bp = MAX_READLEN;

// Maq's default mapping quality
static int DEFAULT_QUAL = 25;

// Number of bases consider "reliable" on the five prime end of each read
static int MAQ_FIVE_PRIME = 28;

template<int MAXLEN>
static inline int operator < (const aln_t<MAXLEN>a, const aln_t<MAXLEN>& b)
{
	return (a.seqid < b.seqid) || (a.seqid == b.seqid && a.pos < b.pos);
}

// A simple way to compute mapping quality.
// Reads are ranked in three tiers, 0 seed mismatches, 1 seed mismatches,
// 2 seed mismatches.  Within each rank, the quality is reduced by the log of
// the number of alternative mappings at that level.
static inline int cal_map_qual(int default_qual,
							   unsigned int seed_mismatches,
							   unsigned int other_occs)
{
	if (seed_mismatches == 0)
		return 3 * default_qual - log_n[other_occs];
	if (seed_mismatches == 1)
		return 2 * default_qual - log_n[other_occs];
	else
		return default_qual - log_n[other_occs];
}

/**
 * My version of the strsep function; strsep is not included in MinGW.
 */
static char *my_strsep(char **stringp, const char delim) {
	char *string = *stringp;
	if(string == NULL) return NULL;
	// We're already pointing to the end of the string; just return
	// NULL to indicate there are no more tokens
	int i = 0;
	if(string[i] == '\0') {
		return NULL;
	}
	// Skip over any initial delimiters
	while(string[i] == delim) i++;
	string = &string[i];
	i = 0;
	// Skip to nearest delimiter or terminator
	while(string[i] != delim && string[i] != '\0') {
		i++;
	}
	// Now string[i] is either a delimiter or a terminator
	// Overwrite delimiters with terminators
	while(string[i] == delim) {
		string[i] = '\0';
		i++;
	}
	// Now string[i] either points just past the last delimiter seen,
	// or it points to the existing terminator
	*stringp = &string[i];
#ifndef NDEBUG
	{
		int j = 0;
		while(string[j] != '\0') {
			// No delimiters in the middle of string
			assert_neq(delim, string[j]);
			j++;
		}
	}
#endif
	return string;
}

/** This function, and the types aln_t and header_t, are parameterized by an
 *  integer specifying the maximum read length supported by the Maq map to be
 *  written
 */

template<int MAXLEN>
int convert_bwt_to_maq(const string& bwtmap_fname,
					   const string& maqmap_fname,
					   const map<string, unsigned int>& names_to_ids)
{
	FILE* bwtf = fopen(bwtmap_fname.c_str(), "r");

	if (!bwtf)
	{
		fprintf(stderr, "Error: could not open Bowtie mapfile %s for reading\n", bwtmap_fname.c_str());
		throw 1;
	}

	void* maqf = gzopen(maqmap_fname.c_str(), "w");
	if (!maqf)
	{
		fprintf(stderr, "Error: could not open Maq mapfile %s for writing\n", maqmap_fname.c_str());
		throw 1;
	}

	std::map<string, int> seqid_to_name;
	char bwt_buf[2048];
	static const int buf_size = 256;
	char *name;
	uint32_t seqid = 0;

	// Initialize a new Maq map table
	header_t<MAXLEN> *mm = maq_init_header<MAXLEN>();

	char orientation;
	char *text_name;
	unsigned int text_offset;
	char *sequence;

	char *qualities;
	unsigned int other_occs;
	char *mismatches = NULL;

	int max = 0;
	uint32_t max_incr = 0x100000;

	while (fgets(bwt_buf, 2048, bwtf))
	{
		char* nl = strrchr(bwt_buf, '\n');
		char* buf = bwt_buf;
		if (nl) *nl = 0;

		/**
		 Fields are:
			1) name (or for now, Id)
			2) orientations ('+'/'-')
			3) text name
			4) text offset
			5) sequence of hit (i.e. trimmed read)
		    6) quality values of sequence (trimmed)
			7) # of other hits in EBWT
			8) mismatch positions - this is a comma-delimited list of positions
				w.r.t. the 5 prime end of the read.
		 */
		name                   = my_strsep((char**)&buf, '\t');
		char *scan_orientation = my_strsep((char**)&buf, '\t');
		text_name              = my_strsep((char**)&buf, '\t');
		char *scan_refoff      = my_strsep((char**)&buf, '\t');
		sequence               = my_strsep((char**)&buf, '\t');
		qualities              = my_strsep((char**)&buf, '\t');
		char *scan_oms         = my_strsep((char**)&buf, '\t');
		mismatches             = my_strsep((char**)&buf, '\t');
		if(scan_oms == NULL) {
			// One or more of the fields up to the 'oms' field didn't exist
			fprintf(stderr, "Warning: found malformed record, skipping\n");
			continue;
		}
		// All fields through 'oms' should exist
		assert(name             != NULL &&
		       scan_orientation != NULL &&
		       text_name        != NULL &&
		       scan_refoff      != NULL &&
		       sequence         != NULL &&
		       qualities        != NULL &&
		       scan_oms         != NULL);
		// Parse orientation, text offset and oms
		orientation = *scan_orientation;
		assert(orientation == '+' || orientation == '-');
		// Truncate the name at the first instance of whitespace to be
		// compatible with what fasta2bfa does.
		char *nc = text_name;
		while(*nc != '\0' && !isspace(*nc)) nc++;
		*nc = '\0';
		text_offset = (unsigned int)strtol(scan_refoff, (char**)NULL, 10);
		other_occs  = (unsigned int)strtol(scan_oms,    (char**)NULL, 10);

		if (mm->n_mapped_reads == (bit64_t)max)
		{
			int next_max = max + max_incr;
			if(next_max < max) {
				next_max = INT_MAX;
			}
			aln_t<MAXLEN>* tmp =
				(aln_t<MAXLEN>*)malloc(sizeof(aln_t<MAXLEN>) * (next_max));
			if(tmp == NULL) {
				cerr << "Memory exhausted allocating space for alignments." << endl;
				throw 1;
			}
			memcpy(tmp, mm->mapped_reads, sizeof(aln_t<MAXLEN>) * (max));
			free(mm->mapped_reads);
			mm->mapped_reads = tmp;
			max = next_max;
			if(max_incr < (max_incr + (max_incr >> 1))) {
				max_incr += (max_incr >> 1); // Add 50%
			}
		}

		aln_t<MAXLEN> *m1 = mm->mapped_reads + mm->n_mapped_reads;
		// If the read name exceeds the maximum, cut characters off
		// the beginning rather than the end so that the /1,/2 suffix
		// is maintained.
		size_t name_off = 0;
		if(strlen(name) > max_read_name-1) {
			name_off += (strlen(name) - max_read_name + 1);
		}
		strncpy(m1->name, name + name_off, max_read_name-1);
		m1->name[max_read_name-1] = 0;

		text_name[buf_size-1] = '\0';

		// Convert sequence into Maq's bitpacked format
		memset(m1->seq, 0, MAXLEN);
		m1->size = strlen(sequence);

		map<string, unsigned int>::const_iterator i_text_id =
			names_to_ids.find(text_name);
		if (i_text_id == names_to_ids.end())
		{
			fprintf(stderr, "Warning: read maps to text %s, which is not in BFA, skipping\n", text_name);
			if(verbose) {
				// Print all the reference names
				cerr << "  Reference names in bfa:" << endl;
			    map<string, unsigned int>::const_iterator iter;
			    for(iter = names_to_ids.begin(); iter != names_to_ids.end(); ++iter ) {
			      cerr << "  " << iter->first << endl;
			    }
			}
			continue;
		}

		m1->seqid = i_text_id->second;

		int qual_len = strlen(qualities);

		for (int i = 0; i != m1->size; ++i)
		{
			int tmp = nst_nt4_table[(int)sequence[i]];
			m1->seq[i] = (tmp > 3)? 0 :
			(tmp <<6 | (i < qual_len ? (qualities[i] - 33) & 0x3f : 0));
		}

		vector<string> mismatch_tokens;
		if(mismatches != NULL) {
			tokenize(mismatches, ",", mismatch_tokens);
		}

		vector<int> mis_positions;
		int five_prime_mismatches = 0;
		int three_prime_mismatches = 0;
		int seed_mismatch_quality_sum = 0;
		for (unsigned int i = 0; i < mismatch_tokens.size(); ++i)
		{
			// Chop off everything from after the number on
			for(size_t j = 0; j < mismatch_tokens[i].length(); j++) {
				if(!isdigit(mismatch_tokens[i][j])) {
					mismatch_tokens[i].erase(j);
					break;
				}
			}
			mis_positions.push_back(atoi(mismatch_tokens[i].c_str()));
			int pos = mis_positions.back();
			if (pos < MAQ_FIVE_PRIME)
			{
				// Mismatch positions are always with respect to the 5' end of
				// the read, regardless of whether the read mapping is antisense
				if (orientation == '+')
					seed_mismatch_quality_sum += qualities[pos] - 33;
				else
					seed_mismatch_quality_sum += qualities[m1->size - pos - 1] - 33;

				++five_prime_mismatches;
			}
			else
			{
				// Maq doesn't care about qualities of mismatching bases beyond
				// the seed
				++three_prime_mismatches;
			}
		}

		// Only have 256 entries in the log_n table, so we need to pin other_occs
		other_occs = min(other_occs, 255u);

		m1->c[0] = m1->c[1] = 0;
		if (three_prime_mismatches + five_prime_mismatches)
			m1->c[1] = other_occs + 1; //need to include this mapping as well!
		else
			m1->c[0] = other_occs + 1; //need to include this mapping as well!

		// Unused paired-end data
		m1->flag = 0;
		m1->dist = 0;

		m1->pos = (text_offset << 1) | (orientation == '+'? 0 : 1);

		// info1's high 4 bits are the # of seed mismatches/
		// the low 4 store the total # of mismatches
		m1->info1 = (five_prime_mismatches << 4) |
			(three_prime_mismatches + five_prime_mismatches);

		// Sum of qualities in seed mismatches
		seed_mismatch_quality_sum = ((seed_mismatch_quality_sum <= 0xff) ?
									 seed_mismatch_quality_sum : 0xff);
		m1->info2 = seed_mismatch_quality_sum;

		m1->map_qual = cal_map_qual(DEFAULT_QUAL,
									five_prime_mismatches,
									other_occs);

		m1->seq[MAXLEN-1] = m1->alt_qual = m1->map_qual;

		++mm->n_mapped_reads;
		seqid++; // increment unique id for reads
	}

	mm->n_ref = names_to_ids.size();
	mm->ref_name = (char**)malloc(sizeof(char*) * mm->n_ref);
	if(mm->ref_name == NULL) {
		cerr << "Exhausted memory allocating reference name" << endl;
		throw 1;
	}

	for (map<string, unsigned int>::const_iterator i = names_to_ids.begin();
		 i != names_to_ids.end(); ++i)
	{
		char* name = strdup(i->first.c_str());
		if (name)
			mm->ref_name[i->second] = name;
	}

	algo_sort(mm->n_mapped_reads, mm->mapped_reads);

	if (verbose) {
		cerr << "Converted " << mm->n_mapped_reads << " alignments, writing Maq map" << endl;
	}
	// Write out the header
	maq_write_header(maqf, mm);
	// Write out the alignments
	gzwrite(maqf, mm->mapped_reads, sizeof(aln_t<MAXLEN>) * mm->n_mapped_reads);

	maq_destroy_header(mm);

	fclose(bwtf);
	gzclose(maqf);
	return 0;
}

void init_log_n()
{
	log_n[0] = -1;
	for (int i = 1; i != 256; ++i)
		log_n[i] = (int)(3.434 * log(i) + 0.5);
}

void get_names_from_bfa(const string& bfa_filename,
				   map<string, unsigned int>& names_to_ids)
{
	FILE* bfaf = fopen(bfa_filename.c_str(), "r");

	if (!bfaf)
	{
		fprintf(stderr, "Error: could not open Binary FASTA file %s for reading\n", bfa_filename.c_str());
		throw 1;
	}

	unsigned int next_id = 0;
	nst_bfa1_t *l;

	while ((l = nst_load_bfa1(bfaf)) != 0)
	{
		names_to_ids[l->name] = next_id++;
		if (verbose)
		{
			fprintf(stderr, "Reading record for %s from .bfa\n", l->name);
		}
	}
}

int main(int argc, char **argv)
{
	try {
		string bwtmap_filename;
		string maqmap_filename;
		string bfa_filename;
		const char *short_options = "voh";
		int next_option;
		do {
			next_option = getopt(argc, argv, short_options);
			switch (next_option) {
				case 'v': /* verbose */
					verbose = true;
					break;
				case 'o':
					short_map_format = true;
					break;
				case 'h':
					printLongUsage(cout);
					throw 0;
				case -1: /* Done with options. */
					break;
				default:
					printUsage(cerr);
					throw 1;
			}
		} while(next_option != -1);

		if(optind >= argc) {
			printUsage(cerr);
			return 1;
		}

		// The Bowtie output text file to be converted
		bwtmap_filename = argv[optind++];

		if(optind >= argc) {
			printUsage(cerr);
			return 1;
		}

		// The name of the binary Maq map to be written
		maqmap_filename = argv[optind++];

		// An optional argument:
		// a two-column text file of [Bowtie ref id, reference name string] pairs
		if(optind >= argc)
		{
			printUsage(cerr);
			return 1;
		}
		bfa_filename = string(argv[optind++]);

		init_log_n();

		map<string, unsigned int> names_to_ids;
		get_names_from_bfa(bfa_filename, names_to_ids);

		int ret;
		if (short_map_format)
		{
			ret = convert_bwt_to_maq<64>(bwtmap_filename,
										 maqmap_filename,
										 names_to_ids);
		}
		else
		{
			ret = convert_bwt_to_maq<128>(bwtmap_filename,
										 maqmap_filename,
										 names_to_ids);
		}

		return ret;
	} catch(std::exception& e) {
		cerr << "Command: ";
		for(int i = 0; i < argc; i++) cerr << argv[i] << " ";
		cerr << endl;
		return 1;
	} catch(int e) {
		if(e != 0) {
			cerr << "Command: ";
			for(int i = 0; i < argc; i++) cerr << argv[i] << " ";
			cerr << endl;
		}
		return e;
	}
}
