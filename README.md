# HITB PRO CTF 2019

PRO CTF is an onsite international challenge in information security. Developed by Hackerdom team for HITB CyberWeek in Abu Dhabi, UAE. PRO CTF was held on October 15—17 October 2019.

The contest is driven by almost classic rules for Attack-Defense [CTF](https://en.wikipedia.org/wiki/Capture_the_flag#Computer_security). Each team is given a set of vulnerable services.
Organizers regularly fill services with private information — the flags.
The goal of each team is to find vulnerabilities, fix them in their services and exploit them to get flags from other teams.

This repository contains:

* source of all services in folder [services/](https://github.com/HackerDom/proctf-2019/tree/master/services/)
* checkers for [checksystem](https://github.com/Hackerdom/checksystem) in folder [checkers/](checkers/)
* exploits for all services in folder [sploits/](https://github.com/HackerDom/proctf-2019/tree/master/sploits/)
* writeups with vulnerabilities and exploitation description for all services in folder [writeups/](https://github.com/HackerDom/proctf-2019/tree/master/writeups/)

Also, we bring to you some of our internal infrastructure magic:
* deploy scripts for virtual machines with services. Each service was hosted inside docker contrainer inside separate VirtualBox virtual machine. See [deploy/](https://github.com/HackerDom/proctf-2019/tree/master/deploy/)
* switches configuration in [switches/](https://github.com/HackerDom/proctf-2019/tree/master/switches/)
* checksystem configuration in [cs/](https://github.com/HackerDom/proctf-2019/tree/master/cs/)

This CTF is brought to you by these amazing guys:

* Alexander Bersenev aka bay, author of services `fraud_detector`, `polyfill`, and `startup`, also our network and infrastructure master
* Andrew Gein aka andgein, author of service `drone_racing`
* Andrey Khozov aka and, author of services `bb` and `ca`, also our checksystem master
* Artem Zinenko aka art, author of services `deer` and `sql_demo`
* Artur Khanov aka awengar, author of services `Binder` and `Spaceships`
* Dmitry Simonov aka dimmo, author of service `tracker`
* Dmitry Titarenko aka dscheg, author of services `rubik` and `notepool`
* domi, author of service `geocaching`
* Konstantin Plotnikov aka host, author of services `SePtoN` and `gallery`, also our teamleader
* Mikhail Vytskov aka mik, author of service `handy`
* Polina Zonova aka bosonojka5, our amazing project leader and manager
* Ruslan Kutdusov, author of services `game_console` and `convolution`
* Roman Bykov, author of service `dotFM`

If you have any question about services write us an email to info@hackerdom.ru

© 2019 [HackerDom](http://hackerdom.ru)
