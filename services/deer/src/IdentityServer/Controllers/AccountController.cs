using System.ComponentModel.DataAnnotations;
using System.Linq;
using System.Security.Claims;
using System.Threading.Tasks;
using IdentityServer.Helpers;
using IdentityServer.Repositories;
using Microsoft.AspNetCore.Authentication;
using Microsoft.AspNetCore.Authentication.Cookies;
using Microsoft.AspNetCore.Mvc;

namespace IdentityServer.Controllers
{
    public class AccountController : Controller
    {
        private readonly IUserUnitOfWork _userUnitOfWork;
        private readonly IUserRepository _userRepository;

        public AccountController(IUserUnitOfWork userUnitOfWork, IUserRepository userRepository)
        {
            _userUnitOfWork = userUnitOfWork;
            _userRepository = userRepository;
        }

        [HttpGet]
        public IActionResult Login()
        {
            return View();
        }

        [HttpPost]
        public async Task<IActionResult> Login([Required] string username, [Required] string password)
        {
            if (ModelState.IsValid)
            {
                var user = await _userRepository.GetAsync(username);
                if (user != null)
                {
                    var passwordHash = CryptoUtils.ComputeHash(user.Salt, password);
                    if (user.PasswordHash.SequenceEqual(passwordHash))
                        return await LoginUser(username);
                }
            }

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
        public async Task<IActionResult> SignUp([RegularExpression("^[a-zA-Z0-9]{1,50}$")] string username, string password)
        {
            if (ModelState.IsValid && await _userUnitOfWork.CreateUserAsync(username, password))
            {
                return await LoginUser(username);
            }
            
            return View();
        }

        public async Task<IActionResult> Logout()
        {
            await HttpContext.SignOutAsync(CookieAuthenticationDefaults.AuthenticationScheme);
            return RedirectToAction("Login");
        }
    }
}