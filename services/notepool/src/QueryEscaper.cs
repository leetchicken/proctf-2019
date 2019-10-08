using System.Collections.Generic;
using System.Text;
using System.Text.RegularExpressions;

namespace notepool
{
	public static class QuerySanitizer
	{
		public static string EscapeText(this string text)
			=> text == null ? null : EscapeSpecialChars(text).EscapeKeywords().Trim();

		public static string EscapeKeyword(this string word)
			=> word != null && EscapeWords.Contains(word) ? word.ToLower() : word;

		private static string EscapeKeywords(this string text)
			=> text == null ? null : WordRegex.Replace(text, match => match.Value.EscapeKeyword());

		private static string EscapeSpecialChars(string text)
		{
			var builder = new StringBuilder(text.Length << 1);
			for(int index = 0; index < text.Length; ++index)
			{
				var c = text[index];
				if(char.IsWhiteSpace(c))
					builder.Append(' ');
				else
				{
					if(EscapeChars.Contains(c))
						builder.Append('\\');
					builder.Append(c);
				}
			}
			return builder.ToString();
		}

		private static readonly Regex WordRegex = new Regex(@"\w+", RegexOptions.Compiled);
		private static readonly HashSet<char> EscapeChars = new HashSet<char>("+-@*?<>=!()^[{:]}~/\\".ToCharArray());
		private static readonly HashSet<string> EscapeWords = new HashSet<string> {"AND", "OR", "NOT", "TO", "WITHIN", "SENTENCE", "PARAGRAPH", "INORDER"};
	}
}
