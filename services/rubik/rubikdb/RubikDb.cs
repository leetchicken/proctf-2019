using System;
using System.Collections.Generic;

namespace rubikdb
{
	public class User
	{
		public string Login { get; set; }
		public string Pass { get; set; }
		public string Bio { get; set; }
		public DateTime Created { get; set; }
	}

	public class Solution
	{
		public Guid Id { get; set; }
		public string Login { get; set; }
		public int MovesCount { get; set; }
		public ulong Time { get; set; }
		public DateTime Created { get; set; }
	}

	public class RubikDb
	{
		public static User FindUser(string login)
			=> login == null ? null : UserIndex.GetOrDefault(login);

		public static User GetOrAddUser(User item)
		{
			if(UserIndex.GetOrAdd(ref item))
				UserStore.Write(item);
			return item;
		}

		public static bool TryAddSolution(Solution item)
		{
			if(!SolutionIndex.TryAdd(item))
				return false;
			SolutionStore.Write(item);
			return true;
		}

		public static IEnumerable<Solution> TakeSolutions() => SolutionIndex.TakeLast();

		public static void Close()
		{
			UserStore.Flush();
			SolutionStore.Flush();
		}

		private static readonly RubikIndex<string, User> UserIndex = new RubikIndex<string, User>(TimeSpan.FromMinutes(60), user => user.Login, null, StringComparer.Ordinal);
		private static readonly RubikStore<User> UserStore = new RubikStore<User>("data/users.db", user => UserIndex.TryAdd(user, user.Created));

		private static readonly RubikIndex<Guid, Solution> SolutionIndex = new RubikIndex<Guid, Solution>(TimeSpan.FromMinutes(30), sln => sln.Id);
		private static readonly RubikStore<Solution> SolutionStore = new RubikStore<Solution>("data/solutions.db", sln => SolutionIndex.TryAdd(sln, sln.Created));
	}
}