using Microsoft.AspNetCore.Mvc;

namespace UserService.Controllers
{
    public class HomeController : Controller
    {
        public IActionResult Index()
        {
            return View();
        }
    }
}