using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Lucene.Net.Analysis;
using Lucene.Net.Analysis.Miscellaneous;
using Lucene.Net.Documents;
using Lucene.Net.Index;
using Lucene.Net.QueryParsers.Classic;
using Lucene.Net.Search;
using Lucene.Net.Store;
using Lucene.Net.Util;
using notepool.users;

namespace notepool
{
	internal static class LuceneIndex
	{
		static LuceneIndex()
		{
			System.IO.Directory.CreateDirectory(IndexDirectory);
			directory = new MMapDirectory(IndexDirectory);
			analyzer = new WordAnalyzer(LuceneVersion.LUCENE_48);
			writer = new IndexWriter(directory, new IndexWriterConfig(LuceneVersion.LUCENE_48, analyzer)
			{
				IndexDeletionPolicy = new KeepOnlyLastCommitDeletionPolicy(),
				MergedSegmentWarmer = new SimpleMergedSegmentWarmer(null),
				//UseCompoundFile = false,
				OpenMode = OpenMode.CREATE_OR_APPEND,
				UseReaderPooling = true,
				RAMBufferSizeMB = 1024 * 1024
			});
			trackingWriter = new TrackingIndexWriter(writer);

			searcherKeeper = new SearcherManager(writer, false, null);

			reopenThread = new ControlledRealTimeReopenThread<IndexSearcher>(trackingWriter, searcherKeeper, 30.0, 0.0)
			{
				Priority = ThreadPriority.BelowNormal,
				IsBackground = true
			};
			Task.Run(() => reopenThread.Run());
			Task.Run(() => {
				while(true)
				{
					Thread.Sleep(10000);
					try { writer.Commit(); } catch {}
				}
			});
		}

		public static long AddNote(User user, Note note, bool isPrivate)
		{
			using var hashAnalyzer = isPrivate && user?.Key != null ? CreateHashAnalyzer(user.Key) : null;
			var gen = trackingWriter.AddDocument(ToDoc(user?.Login, note), hashAnalyzer ?? analyzer);
			searcherKeeper.MaybeRefresh();
			return gen;
		}

		private static Analyzer CreateHashAnalyzer(byte[] key)
		{
			if(key == null)
				return null;
			var hashAnalyzer = new HashWordAnalyzer(key, LuceneVersion.LUCENE_48);
			return new PerFieldAnalyzerWrapper(analyzer, new Dictionary<string, Analyzer> {{"author", hashAnalyzer}, {"title", hashAnalyzer}, {"text", hashAnalyzer}});
		}

		private static readonly FieldType TimeFieldType = new FieldType
		{
			NumericType = NumericType.INT64,
			IsTokenized = false,
			IsIndexed = true,
			IsStored = true
		};

		private static readonly FieldType LoginFieldType = new FieldType
		{
			IsTokenized = false,
			IsIndexed = true,
			IsStored = false
		};

		private static readonly FieldType IndexableTextFieldType = new FieldType
		{
			IsTokenized = true,
			IsIndexed = true,
			IsStored = true,
			IndexOptions = IndexOptions.DOCS_AND_FREQS_AND_POSITIONS
		};

		private static Document ToDoc(string login, Note note) => new Document
		{
			new Field("author", note.Author ?? string.Empty, IndexableTextFieldType),
			new Field("login", login ?? string.Empty, LoginFieldType),
			new Field("title", note.Title ?? string.Empty, IndexableTextFieldType),
			new Field("text", note.Text ?? string.Empty, IndexableTextFieldType),
			new Int64Field("time", note.Time.ToUnixTimeMinutes(), TimeFieldType)
		};

		public static int TotalCount()
		{
			var searcher = searcherKeeper.Acquire();
			try { return searcher.IndexReader.NumDocs; }
			finally { searcherKeeper.Release(searcher); }
		}

		public static List<Note> Search(string text, User user, bool myOnly, DateTime from, DateTime to, int top, long? gen = null)
		{
			var parser = new MultiFieldQueryParser(LuceneVersion.LUCENE_48, SearchFields, analyzer) {DefaultOperator = Operator.AND};

			text = text.TrimToNull().EscapeText() ?? EmptyRequest;
			var login = (myOnly ? user?.Login.EscapeKeyword() : null) ?? EmptyRequest;

			var query = parser.Parse($"(({text}) OR ({text.HashWords(user?.Key)})) login:{login}");

			if(gen != null && gen <= trackingWriter.Generation) reopenThread.WaitForGeneration(gen.Value, 3000);
			var searcher = searcherKeeper.Acquire();

			try
			{
				var filter = NumericRangeFilter.NewInt64Range("time", from.ToUnixTimeMinutes(), to.ToUnixTimeMinutes(), true, true);
				var hits = searcher.Search(query, filter, top <= 0 ? 1 : top, IndexDescSortOrder, false, false);

				return hits.ScoreDocs.Take(top).Select(hit => searcher.Doc(hit.Doc)).Select(doc => new Note
				{
					Author = doc.Get("author"),
					Title = doc.Get("title"),
					Text = doc.Get("text"),
					Time = doc.GetField("time")?.GetInt64Value()?.FromUnixTimeMinutes() ?? default
				}).ToList();
			}
			finally
			{
				searcherKeeper.Release(searcher);
			}
		}

		public static void Close()
		{
			var start = DateTime.UtcNow;
			Console.WriteLine("Closing...");
			writer.Commit();
			writer.Dispose(true);
			directory.Dispose();
			Console.WriteLine("Closed, elapsed {0}...", DateTime.UtcNow - start);
		}

		private const string IndexDirectory = "data/index";

		private const string EmptyRequest = "\"\"";
		private static readonly string[] SearchFields = {"title", "text", "author"};
		private static readonly Sort IndexDescSortOrder = new Sort(new SortField(null, SortFieldType.DOC, true));

		private static readonly Directory directory;
		private static readonly Analyzer analyzer;
		private static readonly IndexWriter writer;
		private static readonly TrackingIndexWriter trackingWriter;
		private static readonly SearcherManager searcherKeeper;
		private static readonly ControlledRealTimeReopenThread<IndexSearcher> reopenThread;
	}
}
