# Steppatron
SPPuRV 2 Predmetni projekat (Marko Đorđević, Radomir Zlatković, Aleksa Heler)

## Ideja iza Steppatrona
Steper motori će biti povezani na RasPi uz pomoć A4988 drajvera. Kernel modul će upravljati njima kroz GPIO pinove, a kroz parametere se postavlja koliko ih ima i koje pinove koriste. Pisanjem u node se zadaje koju notu želimo da određeni steper "svira" ili se on zaustavlja. Korisnička aplikacija čita MIDI podatke koje šalje klavijatura povezana preko USB-a (verovatno se može uraditi uz pomoc ALSA C biblioteke), obrađuje ih i prosleđuje kernel modulu u zadatom obliku. Pre toga se može implementirati čitanje karaktera sa tastature i njihovo pretvaranje u note. Kasnije se ovo moze proširiti tako da je moguće svirati više tonova u istom trenutku uz pomoć više stepera, i napraviti program koji reprodukuje MIDI fajl umesto direktnog unosa.
