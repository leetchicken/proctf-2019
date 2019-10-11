using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Text;
using System.IO;
using System.Text;

namespace SePtoN_Checker
{
	class FlagsPainter
	{
		public static byte[] DrawFlag(string flag)
		{
			var image = new Bitmap(400, 40, PixelFormat.Format32bppArgb);
			var r = new Random();
			for(int x = 0; x < image.Width; x++)
			{
				for(int y = 0; y < image.Height; y++)
				{
					image.SetPixel(x, y, Color.FromArgb(255, r.Next(256), r.Next(256), r.Next(256)));
				}
			}

			var rectangle = new RectangleF(0, 0, image.Width, image.Height);
			using(Graphics g = Graphics.FromImage(image))
			{
				g.TextRenderingHint = TextRenderingHint.SingleBitPerPixelGridFit;
//				g.FillRectangle(new SolidBrush(Color.Red), 0, 0, image.Width, image.Height);
				StringFormat sf = new StringFormat();
				sf.Alignment = StringAlignment.Center;
				sf.LineAlignment = StringAlignment.Center;
				g.DrawString(flag, new Font(FontFamily.GenericMonospace, 14, FontStyle.Regular), Brushes.White, rectangle, sf);
			}

			var ms = new MemoryStream();
			image.Save(ms, ImageFormat.Bmp);
			return ms.ToArray();
		}
	}
}
