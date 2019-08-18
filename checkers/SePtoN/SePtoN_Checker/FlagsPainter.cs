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
			var image = new Bitmap(300, 25, PixelFormat.Format32bppArgb);
			var rectangle = new RectangleF(0, 0, image.Width, image.Height);
			using(Graphics g = Graphics.FromImage(image))
			{
				g.TextRenderingHint = TextRenderingHint.SingleBitPerPixelGridFit;
				g.FillRectangle(new SolidBrush(Color.Red), 0, 0, image.Width, image.Height);
				StringFormat sf = new StringFormat();
				sf.Alignment = StringAlignment.Center;
				sf.LineAlignment = StringAlignment.Center;
				g.DrawString(flag, new Font(FontFamily.GenericMonospace, 10, FontStyle.Regular), Brushes.White, rectangle, sf);
			}

			var ms = new MemoryStream();
			image.Save(ms, ImageFormat.Bmp);
			return ms.ToArray();
		}
	}
}
