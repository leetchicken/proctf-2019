using System.Threading.Tasks;
using IdentityServer.Repositories;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;

namespace IdentityServer.Controllers
{
    [Authorize]
    public class HomeController : Controller
    {
        private readonly IUserRepository _userRepository;

        public HomeController(IUserRepository userRepository)
        {
            _userRepository = userRepository;
        }

        public async Task<IActionResult> Index()
        {
            var user = await _userRepository.GetAsync(User.Identity.Name);
            return View(user);
        }
    }
}