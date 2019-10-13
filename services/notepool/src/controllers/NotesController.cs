using System;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using Microsoft.Extensions.Caching.Memory;

namespace notepool.controllers
{
	[ApiController]
	public class NotesController : BaseController
	{
		public NotesController(IMemoryCache cache) : base(cache) {}

		[AllowAnonymous]
		[HttpPost("/notes/add")]
		public async Task<IActionResult> AddNote(string title, string text, string isPrivate)
		{
			if(string.IsNullOrEmpty(title) || string.IsNullOrEmpty(text))
				return BadRequest("Title or text is empty");

			if(title.Length > MaxTitleLength || text.Length > MaxTextLength)
				return BadRequest("Title or text is too long");

			var user = await FindUserAsync(User?.Identity?.Name);
			var note = new Note
			{
				Author = user?.Name,
				Title = title,
				Text = text,
				Time = DateTime.UtcNow
			};

			var gen = LuceneIndex.AddNote(user, note, isPrivate == "on");
			Response.Cookies.Append(IndexGenerationCookieName, gen.ToString());

			return Ok("Ok");
		}

		[AllowAnonymous]
		[HttpGet("/notes/search")]
		public async Task<IActionResult> Search(string query, string myOnly)
		{
			if(query?.Length > MaxQueryLength)
				return BadRequest("Query is too long");

			var user = await FindUserAsync(User?.Identity?.Name);

			var to = DateTime.UtcNow;
			var from = to.AddMinutes(-LastMinutesToSearch);
			var gen = Request.Cookies[IndexGenerationCookieName].TryParseOrDefault();

			return Ok(LuceneIndex.Search(query, user, myOnly == "on", from, to, 1000, gen));
		}

		private const int MaxTitleLength = 128;
		private const int MaxTextLength = 512;

		private const int MaxQueryLength = 128;
		private const int LastMinutesToSearch = 30;
		
		private const string IndexGenerationCookieName = "gen";
	}
}
