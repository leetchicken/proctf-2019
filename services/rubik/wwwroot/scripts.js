let $ = (selector) => { return document.querySelector(selector); }

let error = (msg) => {
	let t = $("#errortemplate");
	let err = t.content.querySelector("div.error");
	let id = '_' + Math.random().toString(36).substr(2);
	err.id = id;
	err.textContent = msg || "Unknown error";
	err.style = "opacity:1;animation:fade 3s forwards;";
	let clone = document.importNode(t.content, true);
	$("body").appendChild(clone);
	setTimeout(() => { $("#" + id).remove(); }, 3000);
}

let themesigns = ['\u2605', '\u2600'];
function settheme() {
	let val = (Number(localStorage.getItem("night")) % 2);
	document.body.style = "transition:filter 500ms linear;filter:invert(" + val + ");";
	$("#daynight").textContent = themesigns[val];
}
function toggletheme() {
	localStorage.setItem("night", localStorage.getItem("night") ^ 1);
	settheme();
}
settheme();
$("#daynight").onclick = () => toggletheme();

var x = 0, y = 0;
var curX = 0, curY = 0;

var anitimer = null;

let anisigns = ['\u2B6E', '\u25FC'];
let setani = () => {
	let val = (Number(localStorage.getItem("animation")) % 2);
	if(!val) $(".container").className = "container"; else $(".container").className = "container paused";
	$("#animation").textContent = anisigns[val ^ 1];
}
let toggleani = () => {
	localStorage.setItem("animation", localStorage.getItem("animation") ^ 1);
	setani();
}
$("#animation").onclick = () => toggleani();
setani();

document.body.onmousedown = function(e) {
	var element = e.target || e.srcElement;
	if(element instanceof HTMLInputElement || element instanceof HTMLAnchorElement || !!element.closest(".panel") || !!element.closest(".panel") || !!element.closest(".controller table")) {
		x = 0;y = 0;
		return;
	}
	if(!!anitimer) {
		clearTimeout(anitimer);
		delete anitimer;
		anitimer = null;
	}
	x = e.pageX;
	y = e.pageY;
}
document.body.onmousemove = function(e) {
	if(x === 0 && y === 0)
		return;
	if(e.buttons !== 1)
		return;
	$('.container').style = "transform:rotateX(" + (curY + (y - e.pageY) / 4.0) + "deg) rotateY(" + (curX + (e.pageX - x) / 4.0) + "deg)";
	$(".container").className = "container paused";
};
document.body.onmouseup = function(e) {
	if(x === 0 && y === 0)
		return;
	curX = curX + (e.pageX - x) / 4.0;
	curY = curY + (y - e.pageY) / 4.0;
	if(!localStorage.getItem("animation"))
		anitimer = setTimeout(() => { $(".container").className = "container"; }, 5000);
}

var auth = false;
fetch("/api/auth", {"credentials":"same-origin"}).then(response => {
	if(!response.ok && response.status !== 401) response.text().then(text => { error(text); });
	else if(response.status === 200 && response.headers.get("Content-Type").startsWith("application/json")) {
		response.json().then(json => {
			$("#login").value = json.login || "";
			$("#pass").value = json.pass || "********";
			$("#bio").value = json.bio || "";
			$(".auth").querySelectorAll("input").forEach(input => input.readOnly = true);
			auth = true;
		}).catch(() => error("/api/auth failed"));
	}
});

let startTime = null;
let timer = null;

let cube = null;

let stopped = false;
$("#submit").disabled = true;

let settime = (val) => {
	let value = String((val / 1000.0).toFixed(1)).split('.');
	$("#timesec").textContent = value[0];
	$("#timemsec").textContent = value[1];
};

let clear = () => {
	cube = "RRRRRRRRRGGGGGGGGGBBBBBBBBBWWWWWWWWWYYYYYYYYYZZZZZZZZZ".split('');
	if(!!timer) {
		clearTimeout(timer);
		delete timer;
	}
	$("#solution").value = "";
	settime(0.0);
}

let rotate = (r) => {
	if(r.isUpper())
		rotateClockwise(r);
	else {
		for(var i = 0; i < 3; i++)
			rotateClockwise(r.toUpperCase());
	}
}

let rotateClockwise = (r) => {
	let idx = 0;
	if(r == 'L') {
		idx=0;
		let t1=cube[9];let t2=cube[16];let t3=cube[15];
		cube[9]=cube[18];cube[16]=cube[25];cube[15]=cube[24];
		cube[18]=cube[40];cube[25]=cube[39];cube[24]=cube[38];
		cube[40]=cube[45];cube[39]=cube[52];cube[38]=cube[51];
		cube[45]=t1;cube[52]=t2;cube[51]=t3;
	} else if(r == 'F') {
		idx=1;
		let t1=cube[4];let t2=cube[3];let t3=cube[2];
		cube[2]=cube[45];cube[3]=cube[46];cube[4]=cube[47];
		cube[45]=cube[33];cube[46]=cube[34];cube[47]=cube[27];
		cube[27]=cube[24];cube[34]=cube[23];cube[33]=cube[22];
		cube[24]=t1;cube[23]=t2;cube[22]=t3;
	} else if(r == 'U') {
		idx=2;
		let t1=cube[0];let t2=cube[1];let t3=cube[2];
		cube[0]=cube[9];cube[1]=cube[10];cube[2]=cube[11];
		cube[9]=cube[27];cube[10]=cube[28];cube[11]=cube[29];
		cube[27]=cube[36];cube[28]=cube[37];cube[29]=cube[38];
		cube[36]=t1;cube[37]=t2;cube[38]=t3;
	} else if(r == 'R') {
		idx=3;
		let t1=cube[11];let t2=cube[12];let t3=cube[13];
		cube[11]=cube[47];cube[12]=cube[48];cube[13]=cube[49];
		cube[47]=cube[42];cube[48]=cube[43];cube[49]=cube[36];
		cube[42]=cube[20];cube[43]=cube[21];cube[36]=cube[22];
		cube[20]=t1;cube[21]=t2;cube[22]=t3;
	} else if(r == 'B') {
		idx=4;
		let t1=cube[0];let t2=cube[7];let t3=cube[6];
		cube[0]=cube[20];cube[7]=cube[19];cube[6]=cube[18];
		cube[20]=cube[31];cube[19]=cube[30];cube[18]=cube[29];
		cube[31]=cube[51];cube[30]=cube[50];cube[29]=cube[49];
		cube[51]=t1;cube[50]=t2;cube[49]=t3;
	} else if(r == 'D') {
		idx=5;
		let t1=cube[6];let t2=cube[5];let t3=cube[4];
		cube[6]=cube[42];cube[5]=cube[41];cube[4]=cube[40];
		cube[42]=cube[33];cube[41]=cube[32];cube[40]=cube[31];
		cube[33]=cube[15];cube[32]=cube[14];cube[31]=cube[13];
		cube[15]=t1;cube[14]=t2;cube[13]=t3;
	}
	idx*=9;
	let tmp=cube[idx];
	cube[idx]=cube[idx+6];
	cube[idx+6]=cube[idx + 4];
	cube[idx+4]=cube[idx + 2];
	cube[idx+2]=tmp;
	tmp=cube[idx+1];
	cube[idx+1]=cube[idx+7];
	cube[idx+7]=cube[idx+5];
	cube[idx+5]=cube[idx+3];
	cube[idx+3]=tmp;
}

let repaint = () => {
	for(var i = 0; i < 54; i++)
		$("#c" + i).className = cube[i];
}

String.prototype.toggleCase = function() {
	if(this.isUpper())
		return this.toLowerCase();
	return this.toUpperCase();
}

String.prototype.isUpper = function() {
	if(!this.length) return false;
	for(var i = 0; i < this.length; i++) {
		if(!(this[i] >= 'A' && this[i] <= 'Z'))
			return false;
	}
	return true;
}

document.querySelectorAll(".btn").forEach(btn => {
	btn.onclick = e => {
		if(stopped)
			return;
		let r = btn.id;
		rotate(r);
		repaint();
		let sln = $("#solution").value;
		if(!!sln.length && sln[sln.length - 1] === r.toggleCase())
			$("#solution").value = sln.substr(0, sln.length - 1);
		else
			$("#solution").value += r;
	};
});

let start = () => {
	clear();
	stopped = false;
	startTime = new Date();
	$("#submit").disabled = false;
	timer = setInterval(() => {
		settime(new Date() - startTime);
	}, 30);
}

var puzzle = null;

$("#start").onclick = () => {
	fetch("/api/generate").then(response => {
		if(!response.ok) response.text().then(text => { error(text); });
		else response.json().then(json => {
			start();
			cube = json.rubik.split('');
			puzzle = json.value;
			repaint();
			$("#results").textContent = "";
		});
	}).catch(() => error("/api/generate failed"));
};

let stop = () => {
	stopped = true;
	$("#submit").disabled = true;
	if(!!timer) {
		clearTimeout(timer);
		delete timer;
	}
};

$("#submit").onclick = () => {
	var query = "?puzzle=" + encodeURIComponent(puzzle) + "&solution=" + encodeURIComponent($("#solution").value);
	if(!auth && !!$("#login").value) {
		query += "&login=" + encodeURIComponent($("#login").value);
		query += "&pass=" + encodeURIComponent($("#pass").value);
		query += "&bio=" + encodeURIComponent($("#bio").value);
	}
	fetch("/api/solve" + query, {"credentials":"same-origin", "method":"POST", headers: {"Content-Type": "application/x-www-form-urlencoded"}}).then(response => {
		if(!response.ok) response.text().then(text => { error(text); });
		else response.json().then(json => {
			stop();
			settime(json.time);
			$("#results").textContent = "Good job!\n" + json.movesCount + " moves";
		});
	}).catch(() => error("/api/solve failed"));
}

var page = 0;

let clearScores = () => {
	do {
		var tr = $("#scoreboard tbody tr");
		if(!!tr) tr.remove();
	} while(!!tr);
}

let scores = () => {
	fetch("/api/scoreboard?skip=" + (page * 10000) + "&top=10000").then(response => {
		if(!response.ok) response.text().then(text => { error(text); });
		else response.json().then(json => {
			clearScores();
			$("#page").textContent = page;
			tbody = $("#scoreboard").style = "display:block;";
			tbody = $("#scoreboard tbody");
			for(var i = 0; i < json.length; i++) {
				var t = $("#scoretemplate");
				td = t.content.querySelectorAll("td");
				td[0].textContent = json[i].created && json[i].created.replace(/^.*?T(.*?)\..*$/, "$1");
				td[1].textContent = json[i].login;
				td[2].textContent = json[i].movesCount;
				td[3].textContent = json[i].time && (Number(json[i].time) / 1000.0).toFixed(1);
				var clone = document.importNode(t.content, true);
				tbody.appendChild(clone);
			}
		});
	}).catch(() => error("/api/scoreboard failed"));
};

$("#prev").onclick = () => { if(page > 0) { page--; scores(); } }
$("#next").onclick = () => { if(page < 9) { page++; scores(); } }
$("#close").onclick = () => { $("#scoreboard").style = "display:none"; }

$("#showscores").onclick = () => { page = 0; scores(); }

clear();
repaint();
