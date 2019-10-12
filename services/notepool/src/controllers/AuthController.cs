using System.Collections.Generic;
using System.Security.Claims;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Authentication;
using Microsoft.AspNetCore.Authentication.Cookies;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using Microsoft.Extensions.Caching.Memory;
using notepool.users;

namespace notepool.controllers
{
	[ApiController]
	public class AuthController : BaseController
	{
		public AuthController(IMemoryCache cache) : base(cache) {}

		[AllowAnonymous]
		[HttpPost("/auth/signout")]
		public async Task<IActionResult> SignOut()
		{
			await SignOutAsync();

			return Ok("Ok");
		}

		[AllowAnonymous]
		[HttpPost("/auth/signin")]
		public async Task<IActionResult> SignIn(string login, string password)
		{
			if(string.IsNullOrEmpty(login) || string.IsNullOrEmpty(password))
				return BadRequest("Login or password is empty");

			if(!LoginValidateRegex.IsMatch(login))
				return BadRequest("Bad login");

			using(await AsyncLockPool.GetLockObject(login).AcquireAsync(HttpContext.RequestAborted))
			{
				var user = await UserManager.FindAsync(login);
				if(user == null || !CryptographicOperations.FixedTimeEquals(Encoding.UTF8.GetBytes(user.Password), Encoding.UTF8.GetBytes(password)))
					return StatusCode(403, "No such user or invalid password");

				await SignInAsync(login);
			}

			return Ok("Ok");
		}

		[AllowAnonymous]
		[HttpPost("/auth/signup")]
		public async Task<IActionResult> SignUp(string login, string name, string password)
		{
			if(string.IsNullOrEmpty(login) || string.IsNullOrEmpty(password) || string.IsNullOrEmpty(name))
				return BadRequest("Login or password or name is empty");

			if(!LoginValidateRegex.IsMatch(login))
				return BadRequest("Bad login");

			if(name.Length > MaxFieldLength || password.Length > MaxFieldLength)
				return BadRequest("Field too long");

			using(await AsyncLockPool.GetLockObject(login).AcquireAsync(HttpContext.RequestAborted))
			{
				if(await UserManager.FindAsync(login) != null)
					return Conflict("User already exists");

				if(HttpContext.RequestAborted.IsCancellationRequested)
					return BadRequest("Cancelled");

				var key = new byte[16];
				RandomNumberGenerator.Fill(key);

				var user = new User {Login = login, Name = name, Password = password, Key = key};
				await UserManager.AddAsync(user);

				await SignInAsync(login);
			}

			return Ok("Ok");
		}

		private async Task SignOutAsync() => await HttpContext.SignOutAsync();

		private async Task SignInAsync(string login)
		{
			var principal = new ClaimsPrincipal(new ClaimsIdentity(new List<Claim>{new Claim(ClaimTypes.Name, login)}, CookieAuthenticationDefaults.AuthenticationScheme));
			await HttpContext.SignInAsync(CookieAuthenticationDefaults.AuthenticationScheme, principal);
			AuthHelperMiddleware.UpdateCookie(HttpContext, login);
		}

		private const int MaxFieldLength = 64;
		private static readonly Regex LoginValidateRegex = new Regex(@"^[a-z0-9_]{3,20}$", RegexOptions.Compiled | RegexOptions.IgnoreCase);
	}
}
