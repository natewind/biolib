#ifndef BIOLIB_HPP
#define BIOLIB_HPP

#include <tuple>
#include <string>
#include <iterator>
#include <stdexcept>
#include <algorithm>
#include <string_view>
#include <unordered_map>
#include <pythonized/range>

namespace bio
{
	//========================== Utilities: ===================================

	template <class Range, class T>
	constexpr auto contains(Range const &range, T value) -> bool
	{
		using std::begin, std::end;
		return std::find(begin(range), end(range), value) != end(range);
	}

	template <class Range1, class Range2>
	auto is_valid(Range1 const &str, Range2 const &alphabet) -> bool
	{
		return std::all_of
		(
			std::begin(str), std::end(str),
			[&alphabet](auto const c) { return contains(alphabet, c); }
		);
	}

	constexpr auto complement(char c) -> char
	{
		switch (c)
		{
			case 'A': return 'T';
			case 'C': return 'G';
			case 'G': return 'C';
			case 'T': return 'A';
		}
	}

	constexpr auto pct(float frac) -> float { return 100 * frac; }

	// TODO: Rename to lucas_sequence(n, p = 1, q = -1)
	// TODO: Maybe rewrite using tail recursion
	constexpr auto fibonacci(int n, int a = 1, int b = 1) -> long
	{
		long sum[2] = {1, 1};

		for (auto i : py::range(3, n + 1))
		{
			auto j = bool(i & 1);
			sum[j] = a * sum[j] + b * sum[!j];
		}

		return sum[n & 1];
	}

	//========================== Objects: =====================================

	class FASTA
	{
		std::string id;
		std::string sequence;

	public:
		explicit FASTA() = default;
		void Read(std::istream &stream);

		auto Sequence() const -> std::string const& { return sequence; }
		auto Id() const -> std::string const& { return id; }
	};

	class Strand : public std::string
	{
		Strand &self = *this;

	public:
		using std::string::string;
		explicit Strand(FASTA const &record)
			: std::string(record.Sequence()) {}

		auto Distance(Starnd const &other) const -> int;
		auto ContentGC() const -> float;
		auto Count() const -> std::tuple<int, int, int, int>;
	};

	class RNA;

	class DNA : public Strand
	{
		Strand &self = *this;
		static constexpr char alphabet[] = "ACGT";

	public:
		using Strand::Strand;
		explicit DNA(RNA const &other);
		auto IsValid() const -> bool { return is_valid(self, alphabet); }
		auto Complement() const -> DNA;
	};

	class RNA : public Strand
	{
		Strand &self = *this;
		static constexpr char alphabet[] = "ACGU";

	public:
		using Strand::Strand;
		RNA(DNA const &other);
		auto IsValid() const -> bool { return is_valid(self, alphabet); }
	};

	class Protein : public std::string
	{
		// TODO
	public:
		explicit Protein(RNA const &mRNA);
	};

	//========================== Definitions: =================================

	// TODO: Get rid of the exception, use a named constructor -> std::optional
	void FASTA::Read(std::istream &stream)
	{
		if (stream.peek() != '>')
			throw std::domain_error("Not FASTA format");
		stream.get();
		std::getline(stream, id);

		std::string s;
		for (char c; (c = stream.peek()) != '>' && c != EOF; sequence += s)
			std::getline(stream, s);
	}

	auto Strand::Distance(Strand const &other) const -> int
	{
		auto res = 0;
		auto i = std::begin(other);

		for (char c : self)
		{
			res += (c != *i);
			if (++i == std::end(other))
				break;
		}

		return res;
	}

	auto Strand::ContentGC() const -> float
	{
		constexpr auto is_gc = [](auto c) { return (c == 'G') || (c == 'C'); };

		auto const count = std::count_if(begin(), end(), is_gc);
		return float(count) / length();
	}

	auto Strand::Count() const -> std::tuple<int, int, int, int>
	{
		int N[4] = {0};

		for (auto c : self)
		{
			switch (c)
			{
				case 'A': ++N[0]; break;
				case 'C': ++N[1]; break;
				case 'G': ++N[2]; break;
				default : ++N[3]; break;
			}
		}

		return { N[0], N[1], N[2], N[3] };
	}

	DNA::DNA(RNA const &other) : Strand(other)
	{
		std::replace(std::begin(self), std::end(self), 'U', 'T');
	}

	RNA::RNA(DNA const &other) : Strand(other)
	{
		std::replace(std::begin(self), std::end(self), 'T', 'U');
	}

	auto DNA::Complement() const -> DNA
	{
		auto other = DNA();
		other.reserve(self.length());

		std::transform
		(
			std::rbegin(self), std::rend(self),
			std::back_inserter(other), complement
		);

		return other;
	}

	Protein::Protein(RNA const &mRNA)
	{
		static std::unordered_map<char const*, char> codons =
		{
			{ "UUU", 'F' }, { "CUU", 'L' }, { "AUU", 'I' }, { "GUU", 'V' },
			{ "UUC", 'F' }, { "CUC", 'L' }, { "AUC", 'I' }, { "GUC", 'V' },
			{ "UUA", 'L' }, { "CUA", 'L' }, { "AUA", 'I' }, { "GUA", 'V' },
			{ "UUG", 'L' }, { "CUG", 'L' }, { "AUG", 'M' }, { "GUG", 'V' },
			{ "UCU", 'S' }, { "CCU", 'P' }, { "ACU", 'T' }, { "GCU", 'A' },
			{ "UCC", 'S' }, { "CCC", 'P' }, { "ACC", 'T' }, { "GCC", 'A' },
			{ "UCA", 'S' }, { "CCA", 'P' }, { "ACA", 'T' }, { "GCA", 'A' },
			{ "UCG", 'S' }, { "CCG", 'P' }, { "ACG", 'T' }, { "GCG", 'A' },
			{ "UAU", 'Y' }, { "CAU", 'H' }, { "AAU", 'N' }, { "GAU", 'D' },
			{ "UAC", 'Y' }, { "CAC", 'H' }, { "AAC", 'N' }, { "GAC", 'D' },
			{ "UAA",  0  }, { "CAA", 'Q' }, { "AAA", 'K' }, { "GAA", 'E' },
			{ "UAG",  0  }, { "CAG", 'Q' }, { "AAG", 'K' }, { "GAG", 'E' },
			{ "UGU", 'C' }, { "CGU", 'R' }, { "AGU", 'S' }, { "GGU", 'G' },
			{ "UGC", 'C' }, { "CGC", 'R' }, { "AGC", 'S' }, { "GGC", 'G' },
			{ "UGA",  0  }, { "CGA", 'R' }, { "AGA", 'R' }, { "GGA", 'G' },
			{ "UGG", 'W' }, { "CGG", 'R' }, { "AGG", 'R' }, { "GGG", 'G' }
		};

		// TODO
		auto silence_warning = mRNA.IsValid();
		silence_warning = silence_warning;
	}
}

namespace std
{
	inline auto operator>>(istream &stream, bio::FASTA &arg) -> istream &
	{
		arg.Read(stream);
		return stream;
	}

	inline auto operator<<(ostream &stream, bio::FASTA &arg) -> ostream &
	{
		// TODO

		auto silence_warning = arg;
		silence_warning = silence_warning;

		return stream;
	}
}

#endif
