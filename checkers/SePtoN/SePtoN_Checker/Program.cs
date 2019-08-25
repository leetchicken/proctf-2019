using System;

namespace SePtoN_Checker
{
	class Program
	{
		static int Main(string[] args)
		{
			try
			{
				if(args.Length < 1)
					return ExitWithMessage(ExitCode.CHECKER_ERROR, "Not enough args");

				var mode = args[0].ToLower();
				try
				{
					switch(mode)
					{
						case CommandInfo:
							return ExitWithMessage(ExitCode.OK, null, "vulns: 1");
						case CommandCheck:
							return ProcessCheck(args);
						case CommandPut:
							return ProcessPut(args);
						case CommandGet:
							return ProcessGet(args);
					}
				}
				catch(ServiceException se)
				{
					return ExitWithMessage(se.code, se.ToString(), se.publicMessage);
				}
			}
			catch(Exception e)
			{
				return ExitWithMessage(ExitCode.CHECKER_ERROR, e.ToString());
			}
			return (int)ExitCode.CHECKER_ERROR;
		}

		private static int ProcessCheck(string[] args)
		{
			var host = args[1];
			return ServiceMethods.ProcessCheck(host);
		}

		private static int ProcessPut(string[] args)
		{
			string host, id, flag;
			int vuln;

			int ec;
			if((ec = GetCommonParams(args, out host, out id, out flag, out vuln)) != (int)ExitCode.OK)
				return ec;

			if(vuln == 1)
				return ServiceMethods.ProcessPut(host, id, flag);
			else
				return ExitWithMessage(ExitCode.CHECKER_ERROR, $"Unsupported vuln #{vuln}");
		}

		private static int ProcessGet(string[] args)
		{
			string host, id, flag;
			int vuln;
			GetCommonParams(args, out host, out id, out flag, out vuln);

			return ServiceMethods.ProcessGet(host, id, flag);
		}

		private static int GetCommonParams(string[] args, out string host, out string id, out string flag, out int vuln)
		{
			host = null;
			id = null;
			flag = null;
			vuln = 0;
			if(args.Length < 5 || !int.TryParse(args[4], out vuln))
				return ExitWithMessage(ExitCode.CHECKER_ERROR, "Invalid args");
			host = args[1];
			id = args[2];
			flag = args[3];
			return (int)ExitCode.OK;
		}

		public static int ExitWithMessage(ExitCode exitCode, string stderr, string stdout = null)
		{
			if(stdout != null)
				Console.WriteLine(stdout);
			if(stderr != null)
			{
				if(exitCode == ExitCode.OK)
				{
					Console.WriteLine(stderr);
				}
				else
				{
					Console.Error.WriteLine(stderr);
				}
			}

			return (int)exitCode;
		}

		private const string CommandInfo = "info";
		private const string CommandCheck = "check";
		private const string CommandPut = "put";
		private const string CommandGet = "get";
	}
}
