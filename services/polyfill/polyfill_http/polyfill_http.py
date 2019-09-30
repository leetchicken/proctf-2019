#!/usr/bin/env python3

from aiohttp import web

async def handle_main(request):
    data = open("polyfill.html", encoding="utf8").read()
    return web.Response(text=data, content_type="text/html")


async def handle_js(request):
    data = open("polyfill.js", encoding="utf8").read()
    return web.Response(text=data, content_type="application/javascript")


async def handle_wasm_engine(request):
    data = open("polyfill_engine.wasm", "rb").read()
    return web.Response(body=data, content_type="application/wasm")


async def handle_wasm(request):
    data = open("polyfill.wasm", "rb").read()
    return web.Response(body=data, content_type="application/wasm")


app = web.Application()
app.add_routes([web.get('/', handle_main),
                web.get('/polyfill.js', handle_js),
                web.get('/polyfill_engine.wasm', handle_wasm_engine),
                web.get('/polyfill.wasm', handle_wasm)])

if __name__ == '__main__':
    web.run_app(app)