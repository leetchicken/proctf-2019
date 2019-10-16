# notepool
Notepool is a web service that serve notes online. There are public and private notes with full-text search based on Lucene.net library.

![notepool](img/notepool.png)

The HTTP handlers are:
* `/auth/signup` — user registration;
* `/auth/signin` — user login;
* `/notes/add` — add new notes;
* `/notes/search` — full-text search of added notes;

User can add public notes which can be searched by any other user. Logged in user can search his private notes.
Also user can search through or view all his own notes by using `myOnly` flag.

## Vulns
### Lucene query syntax injection
The main vuln is Lucene query syntax injection through user login which is not fully escaped as one word because of validation.
But there are other vulnerabilities which allows to bypass the validation and use something like `LOGIN OR /[a-z0-9]{32}/` where `/[a-z0-9]{32}/`
is a regular expression which handles hashes of words used to save messages in private. So the full request to Lucene will be
`((SOME TEXT) OR (HASH_OF_SOME HASH_OF_TEXT)) login:LOGIN OR /[a-z0-9]{32}/`.

```cs
var escapedText = text.TrimToNull().EscapeText() ?? EmptyRequest;
var escapedLogin = (myOnly ? user?.Login.EscapeKeyword() : null) ?? EmptyRequest;

var query = parser.Parse($"(({escapedText}) OR ({escapedText.HashWords(user?.Key)})) login:{escapedLogin}");
```
Source: https://github.com/HackerDom/proctf-2019/blob/master/services/notepool/src/LuceneIndex.cs#L118

### Race condition
User profiles are stored in files one field per line. User registration process generates a new user profile with filename equals user login.
But there can be multiple requests at same time which in parallel trying to register a new user. If such happened multiple threads gains
access to one file and write it concurrently because of `FileMode.OpenOrCreate` and `FileShare.ReadWrite`.

Source: https://github.com/HackerDom/proctf-2019/blob/master/services/notepool/src/users/UserManager.cs#L46

There is a lock pool class used to synchronize access to files between threads. The concrete semaphore object is returned by hash of `login`.
But there is an another bug which allows to bypass thread synchronization.

### Unicode handling inconsistency
Logins in service are case-insensitive. To get a filename of the user profile logins are lowercased using current locale.
But the hash of login to get synchronization object is calculated using invariant culture. So to hack it you need to find 2 different chars
that are:
1) matched by [regular expression](https://github.com/HackerDom/proctf-2019/blob/master/services/notepool/src/controllers/AuthController.cs#L95)
`[a-z0-9_]`;
2) [char.ToLower](https://github.com/HackerDom/proctf-2019/blob/master/services/notepool/src/users/UserManager.cs#L29) returns the same char;
3) [InvariantCultureIgnoreCase.GetHashCode](https://github.com/HackerDom/proctf-2019/blob/master/services/notepool/src/AsyncLockPool.cs#L11)
returns the different values;

There is the code to find handling inconsistencies:
```cs
for(int i = 0; i < 0x10ffff; i++)
{
	if(i >= 0x00d800 && i <= 0x00dfff)
		continue;
	var c = char.ConvertFromUtf32(i);
	if(regex.IsMatch(c))
		Console.WriteLine("\\u{0:X4}:{1}:{2}:{3}", i, c, c.ToLower(), c.ToUpper());
}

Regex regex = new Regex("[A-Z]", RegexOptions.Compiled | RegexOptions.IgnoreCase);
```

As a result there are 2 chars that meet all the conditions:
```
\u0130:I:i:I
\u212A:?:k:?
```

So the full exploit is to register concurrently 2 users with diff in one char to get concurrent writes in one file which results in using name as login.
See the exploit here: https://github.com/HackerDom/proctf-2019/blob/master/sploits/notepool/src/Program.cs
