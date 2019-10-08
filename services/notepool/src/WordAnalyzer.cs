using System.IO;
using System.Security.Cryptography;
using Lucene.Net.Analysis;
using Lucene.Net.Analysis.TokenAttributes;
using Lucene.Net.Analysis.Util;
using Lucene.Net.Support;
using Lucene.Net.Util;

namespace notepool
{
	public sealed class WordAnalyzer : Analyzer
	{
		public WordAnalyzer(LuceneVersion matchVersion)
			=> this.matchVersion = matchVersion;

		protected override TokenStreamComponents CreateComponents(string fieldName, TextReader reader)
			=> new TokenStreamComponents(new LowerCaseOrDigitTokenizer(matchVersion, reader));

		private readonly LuceneVersion matchVersion;
	}

	public sealed class HashWordAnalyzer : Analyzer
	{
		public HashWordAnalyzer(byte[] key, LuceneVersion matchVersion)
		{
			this.key = key;
			this.matchVersion = matchVersion;
		}

		protected override TokenStreamComponents CreateComponents(string fieldName, TextReader reader)
		{
			var tokenizer = new LowerCaseOrDigitTokenizer(matchVersion, reader);
			return new TokenStreamComponents(tokenizer, new HashFilter(tokenizer, key));
		}

		private readonly byte[] key;
		private readonly LuceneVersion matchVersion;
	}

	public sealed class LowerCaseOrDigitTokenizer : CharTokenizer
	{
		public LowerCaseOrDigitTokenizer(LuceneVersion matchVersion, TextReader @in)
			: base(matchVersion, @in)
		{
		}

		public LowerCaseOrDigitTokenizer(LuceneVersion matchVersion, AttributeFactory factory, TextReader @in)
			: base(matchVersion, factory, @in)
		{
		}

		protected override int Normalize(int c) => Character.ToLower(c);
		protected override bool IsTokenChar(int c) => c == '_' || Character.IsLetter(c) || char.IsDigit((char)c);
	}

	public sealed class HashFilter : TokenFilter
	{
		internal HashFilter(TokenStream input, byte[] key)
			: base(input)
		{
			termAtt = AddAttribute<ICharTermAttribute>();
			hmac = new HMACMD5(key);
		}

		public override bool IncrementToken()
		{
			if(!m_input.IncrementToken())
				return false;

			var hashWord = hmac.HashWord(termAtt.Buffer, 0, termAtt.Length);
			termAtt.SetEmpty().Append(hashWord);
			return true;
		}

		protected override void Dispose(bool disposing)
		{
			base.Dispose(disposing);
			if(disposing) hmac.Dispose();
		}

		private readonly ICharTermAttribute termAtt;
		private readonly HMACMD5 hmac;
	}
}
