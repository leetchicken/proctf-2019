using System;
using System.Security.Claims;
using System.Threading.Tasks;
using EasyNetQ;
using IdentityServer.Models;
using IdentityServer.Repositories;
using Microsoft.AspNetCore.Authentication;
using Microsoft.AspNetCore.Authentication.Cookies;
using Microsoft.AspNetCore.Mvc;

namespace IdentityServer.Controllers
{
    public class AccountController : Controller
    {
        private readonly IUserRepository _userRepository;
        private readonly IBus _bus;

        public AccountController(IUserRepository userRepository, IBus bus)
        {
            _userRepository = userRepository;
            _bus = bus;
        }

        [HttpGet]
        public IActionResult Login()
        {
            return View();
        }

        [HttpPost]
        public async Task<IActionResult> Login(string username, string password)
        {
            if (username == "admin" && password == "admin")
                return await LoginUser(username);

            return View();
        }

        [HttpGet]
        public IActionResult SignUp()
        {
            return View();
        }

        private async Task<IActionResult> LoginUser(string username)
        {
            var userIdentity = new ClaimsIdentity(CookieAuthenticationDefaults.AuthenticationScheme);
            userIdentity.AddClaim(new Claim(ClaimTypes.Name, username));
            var principal = new ClaimsPrincipal(userIdentity);
            await HttpContext.SignInAsync(CookieAuthenticationDefaults.AuthenticationScheme, principal);
            return RedirectToAction("Index", "Home");
        }

        [HttpPost]
        public async Task<IActionResult> SignUp(string username, string password)
        {
            var queue = _bus.Advanced.QueueDeclareAsync("");

            var user = new User
            {
                Username = username,
                PasswordHash = password,
                LogExchangeName = $"logs.{Guid.NewGuid():D}",
                FeedbackQueueName = queue.Result.Name
            };
            await _userRepository.CreateAsync(user);
            return await LoginUser(username);
        }

        public async Task<IActionResult> Logout()
        {
            await HttpContext.SignOutAsync(CookieAuthenticationDefaults.AuthenticationScheme);
            return RedirectToAction("Login");
        }
    }
}