using System.Threading.Tasks;
using Microsoft.AspNetCore.Mvc;
using Microsoft.Extensions.Caching.Memory;
using notepool.users;

namespace notepool.controllers
{
	public class BaseController : ControllerBase
	{
		public BaseController(IMemoryCache cache)
			=> UserManager = new UserManager(cache);

		protected async Task<User> FindUserAsync(string login)
		{
			if(login == null)
				return null;

			using(await AsyncLockPool.GetLockObject(login).AcquireAsync(HttpContext.RequestAborted))
				return await UserManager.FindAsync(login);
		}

		protected readonly AsyncLockPool AsyncLockPool = new AsyncLockPool();
		protected readonly UserManager UserManager;
	}
}
