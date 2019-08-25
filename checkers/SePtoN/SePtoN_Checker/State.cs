﻿using System.Runtime.Serialization;

namespace SePtoN_Checker
{
	[DataContract]
	class State
	{
		[DataMember(Name = "key")] public string MasterKeyHex;
		[DataMember(Name = "id")] public int FileId;
		[DataMember(Name = "md5")] public string SourceImageMd5Hex;
	}
}
