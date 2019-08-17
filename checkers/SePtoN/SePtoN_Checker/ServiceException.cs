using System;

namespace SePtoN_Checker
{
	public class ServiceException : Exception
	{
		public ExitCode code;
		public string publicMessage;

		public ServiceException(ExitCode code, string privateMes, string publicMessage = null, Exception innerException = null) : base(privateMes, innerException)
		{
			this.code = code;
			this.publicMessage = publicMessage;
		}
	}
}