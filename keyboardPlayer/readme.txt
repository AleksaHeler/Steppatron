Korisnički program na početku pita korisnika da li želi da pusti već uradjenu pesmu/melodiju
(ne pesmu iz fajla nego napisanu u nekom .h fajlu) ili želi da svira note sa tastature.

Ukoliko se odluči za pesmu, program pita korisnika da izabere redni broj
motora koji treba da se pokreće
i nakon toba izabere pesmu.
Korisnik se zatim pita da li želi da nastavi slušanje pesme ili da je prekine.

Ukoliko želi da svira note sa tastature, prvo odabere redni broj motora
koji teba da se pokreće ili da kernel modul sam odluči koji motor treba da se pokrene.
U slučaju da korisnik bira motor onda samo jedna nota može biti svirana u jednom trenutku.

Zatim korsnik bira oktavu i kreće da zadaje note sa tastature (vidi sliku za mapiranje)
Pošto je redni broj motora određen ranije, za slanje note je potreban jedan bajt.
Pritiskom na broj 1 šalje se obroj -1 i prekida se upisivanje sa tastature.



