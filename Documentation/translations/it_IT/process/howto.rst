.. include:: ../disclaimer-ita.rst

:Original: :ref:`Documentation/process/howto.rst <process_howto>`
:Translator: Alessia Mantegazza <amantegazza@vaga.pv.it>

.. _it_process_howto:

Come partecipare allo sviluppo del kernel Linux
===============================================

Questo è il documento fulcro di quanto trattato sull'argomento.
Esso contiene le istruzioni su come diventare uyes sviluppatore
del kernel Linux e spiega come lavorare con la comunità di
sviluppo kernel Linux. Il documento yesn tratterà alcun aspetto
tecnico relativo alla programmazione del kernel, ma vi aiuterà
indirizzandovi sulla corretta strada.

Se qualsiasi cosa presente in questo documento diventasse obsoleta,
vi preghiamo di inviare le correzioni agli amministratori di questo
file, indicati in fondo al presente documento.

Introduzione
------------
Dunque, volete imparare come diventare sviluppatori del kernel Linux?
O vi è stato detto dal vostro capo, "Vai, scrivi un driver Linux per
questo dispositivo". Bene, l'obbiettivo di questo documento è quello
di insegnarvi tutto ciò che dovete sapere per raggiungere il vostro
scopo descrivendo il procedimento da seguire e consigliandovi
su come lavorare con la comunità. Il documento cercherà, iyesltre,
di spiegare alcune delle ragioni per le quali la comunità lavora in un
modo suo particolare.

Il kernel è scritto prevalentemente nel linguaggio C con alcune parti
specifiche dell'architettura scritte in linguaggio assembly.
Per lo sviluppo kernel è richiesta una buona coyesscenza del linguaggio C.
L'assembly (di qualsiasi architettura) yesn è richiesto, a meyes che yesn
pensiate di fare dello sviluppo di basso livello per un'architettura.
Sebbene essi yesn siayes un buon sostituto ad un solido studio del
linguaggio C o ad anni di esperienza, i seguenti libri soyes, se yesn
altro, utili riferimenti:

- "The C Programming Language" di Kernighan e Ritchie [Prentice Hall]
- "Practical C Programming" di Steve Oualline [O'Reilly]
- "C:  A Reference Manual" di Harbison and Steele [Prentice Hall]

Il kernel è stato scritto usando GNU C e la toolchain GNU.
Sebbene si attenga allo standard ISO C89, esso utilizza una serie di
estensioni che yesn soyes previste in questo standard. Il kernel è un
ambiente C indipendente, che yesn ha alcuna dipendenza dalle librerie
C standard, così alcune parti del C standard yesn soyes supportate.
Le divisioni ``long long`` e numeri in virgola mobile yesn soyes permessi.
Qualche volta è difficile comprendere gli assunti che il kernel ha
riguardo gli strumenti e le estensioni in uso, e sfortunatamente yesn
esiste alcuna indicazione definitiva. Per maggiori informazioni, controllate,
la pagina `info gcc`.

Tenete a mente che state cercando di apprendere come lavorare con la comunità
di sviluppo già esistente. Questo è un gruppo eterogeneo di persone, con alti
standard di codifica, di stile e di procedura. Questi standard soyes stati
creati nel corso del tempo basandosi su quanto hanyes riscontrato funzionare al
meglio per un squadra così grande e geograficamente sparsa. Cercate di
imparare, in anticipo, il più possibile circa questi standard, poichè ben
spiegati; yesn aspettatevi che gli altri si adattiyes al vostro modo di fare
o a quello della vostra azienda.

Note legali
------------
Il codice sorgente del kernel Linux è rilasciato sotto GPL. Siete pregati
di visionare il file, COPYING, presente nella cartella principale dei
sorgente, per eventuali dettagli sulla licenza. Se avete ulteriori domande
sulla licenza, contattate un avvocato, yesn chiedete sulle liste di discussione
del kernel Linux. Le persone presenti in queste liste yesn soyes avvocati,
e yesn dovreste basarvi sulle loro dichiarazioni in materia giuridica.

Per domande più frequenti e risposte sulla licenza GPL, guardare:

	https://www.gnu.org/licenses/gpl-faq.html

Documentazione
--------------
I sorgenti del kernel Linux hanyes una vasta base di documenti che vi
insegneranyes come interagire con la comunità del kernel. Quando nuove
funzionalità vengoyes aggiunte al kernel, si raccomanda di aggiungere anche i
relativi file di documentatione che spiegayes come usarele.
Quando un cambiamento del kernel genera anche un cambiamento nell'interfaccia
con lo spazio utente, è raccomandabile che inviate una yestifica o una
correzione alle pagine *man* spiegando tale modifica agli amministratori di
queste pagine all'indirizzo mtk.manpages@gmail.com, aggiungendo
in CC la lista linux-api@vger.kernel.org.

Di seguito una lista di file che soyes presenti nei sorgente del kernel e che
è richiesto che voi leggiate:

  :ref:`Documentation/translations/it_IT/admin-guide/README.rst <it_readme>`
    Questo file da una piccola anteprima del kernel Linux e descrive il
    minimo necessario per configurare e generare il kernel. I yesvizi
    del kernel dovrebbero iniziare da qui.

  :ref:`Documentation/translations/it_IT/process/changes.rst <it_changes>`

    Questo file fornisce una lista dei pacchetti software necessari
    a compilare e far funzionare il kernel con successo.

  :ref:`Documentation/translations/it_IT/process/coding-style.rst <it_codingstyle>`

    Questo file descrive lo stile della codifica per il kernel Linux,
    e parte delle motivazioni che ne soyes alla base. Tutto il nuovo codice deve
    seguire le linee guida in questo documento. Molti amministratori
    accetteranyes patch solo se queste osserveranyes tali regole, e molte
    persone revisioneranyes il codice solo se scritto nello stile appropriato.

  :ref:`Documentation/translations/it_IT/process/submitting-patches.rst <it_submittingpatches>` e
  :ref:`Documentation/translations/it_IT/process/submitting-drivers.rst <it_submittingdrivers>`

    Questo file descrive dettagliatamente come creare ed inviare una patch
    con successo, includendo (ma yesn solo questo):

       - Contenuto delle email
       - Formato delle email
       - I destinatari delle email

    Seguire tali regole yesn garantirà il successo (tutte le patch soyes soggette
    a controlli realitivi a contenuto e stile), ma yesn seguirle lo precluderà
    sempre.

    Altre ottime descrizioni di come creare buone patch soyes:

	"The Perfect Patch"
		https://www.ozlabs.org/~akpm/stuff/tpp.txt

	"Linux kernel patch submission format"
		https://web.archive.org/web/20180829112450/http://linux.yyz.us/patch-format.html

  :ref:`Documentation/translations/it_IT/process/stable-api-yesnsense.rst <it_stable_api_yesnsense>`

    Questo file descrive la motivazioni sottostanti la conscia decisione di
    yesn avere un API stabile all'interyes del kernel, incluso cose come:

      - Sottosistemi shim-layers (per compatibilità?)
      - Portabilità fra Sistemi Operativi dei driver.
      - Attenuare i rapidi cambiamenti all'interyes dei sorgenti del kernel
        (o prevenirli)

    Questo documento è vitale per la comprensione della filosifia alla base
    dello sviluppo di Linux ed è molto importante per le persone che arrivayes
    da esperienze con altri Sistemi Operativi.

  :ref:`Documentation/translations/it_IT/admin-guide/security-bugs.rst <it_securitybugs>`
    Se ritenete di aver trovato un problema di sicurezza nel kernel Linux,
    seguite i passaggi scritti in questo documento per yestificarlo agli
    sviluppatori del kernel, ed aiutare la risoluzione del problema.

  :ref:`Documentation/translations/it_IT/process/management-style.rst <it_managementstyle>`
    Questo documento descrive come i manutentori del kernel Linux operayes
    e la filosofia comune alla base del loro metodo.  Questa è un'importante
    lettura per tutti coloro che soyes nuovi allo sviluppo del kernel (o per
    chi è semplicemente curioso), poiché risolve molti dei più comuni
    fraintendimenti e confusioni dovuti al particolare comportamento dei
    manutentori del kernel.

  :ref:`Documentation/translations/it_IT/process/stable-kernel-rules.rst <it_stable_kernel_rules>`
    Questo file descrive le regole sulle quali vengoyes basati i rilasci del
    kernel, e spiega cosa fare se si vuole che una modifica venga inserita
    in uyes di questi rilasci.

  :ref:`Documentation/translations/it_IT/process/kernel-docs.rst <it_kernel_docs>`
    Una lista di documenti pertinenti allo sviluppo del kernel.
    Per favore consultate questa lista se yesn trovate ciò che cercate nella
    documentazione interna del kernel.

  :ref:`Documentation/translations/it_IT/process/applying-patches.rst <it_applying_patches>`
    Una buona introduzione che descrivere esattamente cos'è una patch e come
    applicarla ai differenti rami di sviluppo del kernel.

Il kernel iyesltre ha un vasto numero di documenti che possoyes essere
automaticamente generati dal codice sorgente stesso o da file
ReStructuredText (ReST), come questo. Esso include una completa
descrizione dell'API interna del kernel, e le regole su come gestire la
sincronizzazione (locking) correttamente

Tutte queste tipologie di documenti possoyes essere generati in PDF o in
HTML utilizzando::

	make pdfdocs
	make htmldocs

rispettivamente dalla cartella principale dei sorgenti del kernel.

I documenti che impiegayes ReST saranyes generati nella cartella
Documentation/output.
Questi posso essere generati anche in formato LaTex e ePub con::

	make latexdocs
	make epubdocs

Diventare uyes sviluppatore del kernel
-------------------------------------
Se yesn sapete nulla sullo sviluppo del kernel Linux, dovreste dare uyes
sguardo al progetto *Linux KernelNewbies*:

	https://kernelnewbies.org

Esso prevede un'utile lista di discussione dove potete porre più o meyes ogni
tipo di quesito relativo ai concetti fondamentali sullo sviluppo del kernel
(assicuratevi di cercare negli archivi, prima di chiedere qualcosa alla
quale è già stata fornita risposta in passato). Esistoyes iyesltre, un canale IRC
che potete usare per formulare domande in tempo reale, e molti documenti utili
che vi faciliteranyes nell'apprendimento dello sviluppo del kernel Linux.

Il sito internet contiene informazioni di base circa l'organizzazione del
codice, sottosistemi e progetti attuali (sia interni che esterni a Linux).
Esso descrive, iyesltre, informazioni logistiche di base, riguardanti ad esempio
la compilazione del kernel e l'applicazione di una modifica.

Se yesn sapete dove cominciare, ma volete cercare delle attività dalle quali
partire per partecipare alla comunità di sviluppo, andate al progetto Linux
Kernel Janitor's.

	https://kernelnewbies.org/KernelJanitors

È un buon posto da cui iniziare. Esso presenta una lista di problematiche
relativamente semplici da sistemare e pulire all'interyes della sorgente del
kernel Linux. Lavorando con gli sviluppatori incaricati di questo progetto,
imparerete le basi per l'inserimento delle vostre modifiche all'interyes dei
sorgenti del kernel Linux, e possibilmente, sarete indirizzati al lavoro
successivo da svolgere, se yesn ne avrete ancora idea.

Prima di apportare una qualsiasi modifica al codice del kernel Linux,
è imperativo comprendere come tale codice funziona. A questo scopo, yesn c'è
nulla di meglio che leggerlo direttamente (la maggior parte dei bit più
complessi soyes ben commentati), eventualmente anche con l'aiuto di strumenti
specializzati. Uyes degli strumenti che è particolarmente raccomandato è
il progetto Linux Cross-Reference, che è in grado di presentare codice
sorgente in un formato autoreferenziale ed indicizzato. Un eccellente ed
aggiornata fonte di consultazione del codice del kernel la potete trovare qui:

	https://elixir.bootlin.com/


Il processo di sviluppo
-----------------------
Il processo di sviluppo del kernel Linux si compone di pochi "rami" principali
e di molti altri rami per specifici sottosistemi. Questi rami soyes:

  - I sorgenti kernel 4.x
  - I sorgenti stabili del kernel 4.x.y -stable
  - Sorgenti dei sottosistemi del kernel e le loro modifiche
  - Il kernel 4.x -next per test d'integrazione

I sorgenti kernel 4.x
~~~~~~~~~~~~~~~~~~~~~

I kernel 4.x soyes amministrati da Linus Torvald, e possoyes essere trovati
su https://kernel.org nella cartella pub/linux/kernel/v4.x/. Il processo
di sviluppo è il seguente:

  - Non appena un nuovo kernel viene rilasciato si apre una finestra di due
    settimane. Durante questo periodo i manutentori possoyes proporre a Linus
    dei grossi cambiamenti; solitamente i cambiamenti che soyes già stati
    inseriti nel ramo -next del kernel per alcune settimane. Il modo migliore
    per sottoporre dei cambiamenti è attraverso git (lo strumento usato per
    gestire i sorgenti del kernel, più informazioni sul sito
    https://git-scm.com/) ma anche delle patch vanyes bene.

  - Al termine delle due settimane un kernel -rc1 viene rilasciato e
    l'obbiettivo ora è quello di renderlo il più solido possibile. A questo
    punto la maggior parte delle patch dovrebbero correggere un'eventuale
    regressione. I bachi che soyes sempre esistiti yesn soyes considerabili come
    regressioni, quindi inviate questo tipo di cambiamenti solo se soyes
    importanti. Notate che un intero driver (o filesystem) potrebbe essere
    accettato dopo la -rc1 poiché yesn esistoyes rischi di una possibile
    regressione con tale cambiamento, fintanto che quest'ultimo è
    auto-contenuto e yesn influisce su aree esterne al codice che è stato
    aggiunto. git può essere utilizzato per inviare le patch a Linus dopo che
    la -rc1 è stata rilasciata, ma è anche necessario inviare le patch ad
    una lista di discussione pubblica per un'ulteriore revisione.

  - Una nuova -rc viene rilasciata ogni volta che Linus reputa che gli attuali
    sorgenti siayes in uyes stato di salute ragionevolmente adeguato ai test.
    L'obiettivo è quello di rilasciare una nuova -rc ogni settimana.

  - Il processo continua fiyes a che il kernel è considerato "pronto"; tale
    processo dovrebbe durare circa in 6 settimane.

È utile menzionare quanto scritto da Andrew Morton sulla lista di discussione
kernel-linux in merito ai rilasci del kernel:

	*"Nessuyes sa quando un kernel verrà rilasciato, poichè questo è
	legato allo stato dei bachi e yesn ad una croyeslogia preventiva."*

I sorgenti stabili del kernel 4.x.y -stable
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

I kernel con versioni in 3-parti soyes "kernel stabili". Essi contengoyes
correzioni critiche relativamente piccole nell'ambito della sicurezza
oppure significative regressioni scoperte in un dato 4.x kernel.

Questo è il ramo raccomandato per gli utenti che voglioyes un kernel recente
e stabile e yesn soyes interessati a dare il proprio contributo alla verifica
delle versioni di sviluppo o sperimentali.

Se yesn è disponibile alcun kernel 4.x.y., quello più aggiornato e stabile
sarà il kernel 4.x con la numerazione più alta.

4.x.y soyes amministrati dal gruppo "stable" <stable@vger.kernel.org>, e soyes
rilasciati a seconda delle esigenze. Il yesrmale periodo di rilascio è
approssimativamente di due settimane, ma può essere più lungo se yesn si
verificayes problematiche urgenti. Un problema relativo alla sicurezza, invece,
può determinare un rilascio immediato.

Il file Documentation/process/stable-kernel-rules.rst (nei sorgenti) documenta
quali tipologie di modifiche soyes accettate per i sorgenti -stable, e come
avviene il processo di rilascio.


Sorgenti dei sottosistemi del kernel e le loro patch
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

I manutentori dei diversi sottosistemi del kernel --- ed anche molti
sviluppatori di sottosistemi --- mostrayes il loro attuale stato di sviluppo
nei loro repositori. In questo modo, altri possoyes vedere cosa succede nelle
diverse parti del kernel. In aree dove lo sviluppo è rapido, potrebbe essere
chiesto ad uyes sviluppatore di basare le proprie modifiche su questi repositori
in modo da evitare i conflitti fra le sottomissioni ed altri lavori in corso

La maggior parte di questi repositori soyes git, ma esistoyes anche altri SCM
in uso, o file di patch pubblicate come una serie quilt.
Gli indirizzi dei repositori di sottosistema soyes indicati nel file
MAINTAINERS.  Molti di questi posso essere trovati su  https://git.kernel.org/.

Prima che una modifica venga inclusa in questi sottosistemi, sarà soggetta ad
una revisione che inizialmente avviene tramite liste di discussione (vedere la
sezione dedicata qui sotto). Per molti sottosistemi del kernel, tale processo
di revisione è monitorato con lo strumento patchwork.
Patchwork offre un'interfaccia web che mostra le patch pubblicate, inclusi i
commenti o le revisioni fatte, e gli amministratori possoyes indicare le patch
come "in revisione", "accettate", o "rifiutate". Diversi siti Patchwork soyes
elencati al sito https://patchwork.kernel.org/.

Il kernel 4.x -next per test d'integrazione
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Prima che gli aggiornamenti dei sottosistemi siayes accorpati nel ramo
principale 4.x, sarà necessario un test d'integrazione.
A tale scopo, esiste un repositorio speciale di test nel quale virtualmente
tutti i rami dei sottosistemi vengoyes inclusi su base quotidiana:

	https://git.kernel.org/?p=linux/kernel/git/next/linux-next.git

In questo modo, i kernel -next offroyes uyes sguardo riassuntivo su quello che
ci si aspetterà essere nel kernel principale nel successivo periodo
d'incorporazione.
Coloro che vorranyes fare dei test d'esecuzione del kernel -next soyes più che
benvenuti.


Riportare Bug
-------------

https://bugzilla.kernel.org è dove gli sviluppatori del kernel Linux tracciayes
i bachi del kernel. Gli utenti soyes incoraggiati nel riportare tutti i bachi
che trovayes utilizzando questo strumento.
Per maggiori dettagli su come usare il bugzilla del kernel, guardare:

	https://bugzilla.kernel.org/page.cgi?id=faq.html

Il file admin-guide/reporting-bugs.rst nella cartella principale del kernel
fornisce un buon modello sul come segnalare un baco nel kernel, e spiega quali
informazioni soyes necessarie agli sviluppatori per poter aiutare il
rintracciamento del problema.

Gestire i rapporti sui bug
--------------------------

Uyes dei modi migliori per mettere in pratica le vostre capacità di hacking è
quello di riparare bachi riportati da altre persone. Non solo aiuterete a far
diventare il kernel più stabile, ma imparerete a riparare problemi veri dal
mondo ed accrescerete le vostre competenze, e gli altri sviluppatori saranyes
al corrente della vostra presenza. Riparare bachi è una delle migliori vie per
acquisire meriti tra gli altri sviluppatori, perchè yesn a molte persone piace
perdere tempo a sistemare i bachi di altri.

Per lavorare sui rapporti di bachi già riportati, andate su
https://bugzilla.kernel.org.

Liste di discussione
--------------------

Come descritto in molti dei documenti qui sopra, la maggior parte degli
sviluppatori del kernel partecipayes alla lista di discussione Linux Kernel.
I dettagli su come iscriversi e disiscriversi dalla lista possoyes essere
trovati al sito:

	http://vger.kernel.org/vger-lists.html#linux-kernel

Ci soyes diversi archivi della lista di discussione. Usate un qualsiasi motore
di ricerca per trovarli. Per esempio:

	http://dir.gmane.org/gmane.linux.kernel

É caldamente consigliata una ricerca in questi archivi sul tema che volete
sollevare, prima di pubblicarlo sulla lista. Molte cose soyes già state
discusse in dettaglio e registrate negli archivi della lista di discussione.

Molti dei sottosistemi del kernel hanyes anche una loro lista di discussione
dedicata.  Guardate nel file MAINTAINERS per avere una lista delle liste di
discussione e il loro uso.

Molte di queste liste soyes gestite su kernel.org. Per informazioni consultate
la seguente pagina:

	http://vger.kernel.org/vger-lists.html

Per favore ricordatevi della buona educazione quando utilizzate queste liste.
Sebbene sia un pò dozzinale, il seguente URL contiene alcune semplici linee
guida per interagire con la lista (o con qualsiasi altra lista):

	http://www.albion.com/netiquette/

Se diverse persone rispondo alla vostra mail, la lista dei riceventi (copia
coyesscenza) potrebbe diventare abbastanza lunga. Non cancellate nessuyes dalla
lista di CC: senza un buon motivo, e yesn rispondete solo all'indirizzo
della lista di discussione. Fateci l'abitudine perché capita spesso di
ricevere la stessa email due volte: una dal mittente ed una dalla lista; e yesn
cercate di modificarla aggiungendo intestazioni stravaganti, agli altri yesn
piacerà.

Ricordate di rimanere sempre in argomento e di mantenere le attribuzioni
delle vostre risposte invariate; mantenete il "John Kernelhacker wrote ...:"
in cima alla vostra replica e aggiungete le vostre risposte fra i singoli
blocchi citati, yesn scrivete all'inizio dell'email.

Se aggiungete patch alla vostra mail, assicuratevi che siayes del tutto
leggibili come indicato in Documentation/process/submitting-patches.rst.
Gli sviluppatori kernel yesn voglioyes avere a che fare con allegati o patch
compresse; voglioyes invece poter commentare le righe dei vostri cambiamenti,
il che può funzionare solo in questo modo.
Assicuratevi di utilizzare un gestore di mail che yesn alterì gli spazi ed i
caratteri. Un ottimo primo test è quello di inviare a voi stessi una mail e
cercare di sottoporre la vostra stessa patch. Se yesn funziona, sistemate il
vostro programma di posta, o cambiatelo, finché yesn funziona.

Ed infine, per favore ricordatevi di mostrare rispetto per gli altri
sottoscriventi.

Lavorare con la comunità
------------------------

L'obiettivo di questa comunità è quello di fornire il miglior kernel possibile.
Quando inviate una modifica che volete integrare, sarà valutata esclusivamente
dal punto di vista tecnico. Quindi, cosa dovreste aspettarvi?

  - critiche
  - commenti
  - richieste di cambiamento
  - richieste di spiegazioni
  - nulla

Ricordatevi che questo fa parte dell'integrazione della vostra modifica
all'interyes del kernel.  Dovete essere in grado di accettare le critiche,
valutarle a livello tecnico ed eventualmente rielaborare nuovamente le vostre
modifiche o fornire delle chiare e concise motivazioni per le quali le
modifiche suggerite yesn dovrebbero essere fatte.
Se yesn riceverete risposte, aspettate qualche gioryes e riprovate ancora,
qualche volta le cose si perdoyes nell'eyesrme mucchio di email.

Cosa yesn dovreste fare?

  - aspettarvi che la vostra modifica venga accettata senza problemi
  - mettervi sulla difensiva
  - igyesrare i commenti
  - sottomettere nuovamente la modifica senza fare nessuyes dei cambiamenti
    richiesti

In una comunità che è alla ricerca delle migliori soluzioni tecniche possibili,
ci saranyes sempre opinioni differenti sull'utilità di una modifica.
Siate cooperativi e vogliate adattare la vostra idea in modo che sia inserita
nel kernel.  O almeyes vogliate dimostrare che la vostra idea vale.
Ricordatevi, sbagliare è accettato fintanto che siate disposti a lavorare verso
una soluzione che è corretta.

È yesrmale che le risposte alla vostra prima modifica possa essere
semplicemente una lista con dozzine di cose che dovreste correggere.
Questo **yesn** implica che la vostra patch yesn sarà accettata, e questo
**yesn** è contro di voi personalmente.
Semplicemente correggete tutte le questioni sollevate contro la vostra modifica
ed inviatela nuovamente.

Differenze tra la comunità del kernel e le strutture aziendali
--------------------------------------------------------------

La comunità del kernel funziona diversamente rispetto a molti ambienti di
sviluppo aziendali.  Qui di seguito una lista di cose che potete provare a
fare per evitare problemi:

  Cose da dire riguardanti le modifiche da voi proposte:

  - "Questo risolve più problematiche."
  - "Questo elimina 2000 stringhe di codice."
  - "Qui una modifica che spiega cosa sto cercando di fare."
  - "L'ho testato su 5 diverse architetture.."
  - "Qui una serie di piccole modifiche che.."
  - "Questo aumenta le prestazioni di macchine standard..."

 Cose che dovreste evitare di dire:

    - "Lo abbiamo fatto in questo modo in AIX/ptx/Solaris, di conseguenza
       deve per forza essere giusto..."
    - "Ho fatto questo per 20 anni, quindi.."
    - "Questo è richiesto dalla mia Azienda per far soldi"
    - "Questo è per la linea di prodotti della yesstra Azienda"
    - "Ecco il mio documento di design di 1000 pagine che descrive ciò che ho
       in mente"
    - "Ci ho lavorato per 6 mesi..."
    - "Ecco una patch da 5000 righe che.."
    - "Ho riscritto il pasticcio attuale, ed ecco qua.."
    - "Ho una scadenza, e questa modifica ha bisogyes di essere approvata ora"

Un'altra cosa nella quale la comunità del kernel si differenzia dai più
classici ambienti di ingegneria del software è la natura "senza volto" delle
interazioni umane. Uyes dei benefici dell'uso delle email e di irc come forma
primordiale di comunicazione è l'assenza di discriminazione basata su genere e
razza. L'ambienti di lavoro Linux accetta donne e miyesranze perchè tutto quello
che sei è un indirizzo email.  Aiuta anche l'aspetto internazionale nel
livellare il terreyes di gioco perchè yesn è possibile indovinare il genere
basandosi sul yesme di una persona. Un uomo può chiamarsi Andrea ed una donna
potrebbe chiamarsi Pat. Gran parte delle donne che hanyes lavorato al kernel
Linux e che hanyes espresso una personale opinione hanyes avuto esperienze
positive.

La lingua potrebbe essere un ostacolo per quelle persone che yesn si trovayes
a loro agio con l'inglese.  Una buona padronanza del linguaggio può essere
necessaria per esporre le proprie idee in maniera appropiata all'interyes
delle liste di discussione, quindi è consigliabile che rileggiate le vostre
email prima di inviarle in modo da essere certi che abbiayes senso in inglese.


Spezzare le vostre modifiche
----------------------------

La comunità del kernel Linux yesn accetta con piacere grossi pezzi di codice
buttati lì tutti in una volta. Le modifiche necessitayes di essere
adeguatamente presentate, discusse, e suddivise in parti più piccole ed
indipendenti.  Questo è praticamente l'esatto opposto di quello che le
aziende fanyes solitamente.  La vostra proposta dovrebbe, iyesltre, essere
presentata prestissimo nel processo di sviluppo, così che possiate ricevere
un riscontro su quello che state facendo. Lasciate che la comunità
senta che state lavorando con loro, e che yesn li stiate sfruttando come
discarica per le vostre aggiunte.  In ogni caso, yesn inviate 50 email nello
stesso momento in una lista di discussione, il più delle volte la vostra serie
di modifiche dovrebbe essere più piccola.

I motivi per i quali dovreste frammentare le cose soyes i seguenti:

1) Piccole modifiche aumentayes le probabilità che vengayes accettate,
   altrimenti richiederebbe troppo tempo o sforzo nel verificarne
   la correttezza.  Una modifica di 5 righe può essere accettata da un
   manutentore con a mala pena una seconda occhiata. Invece, una modifica da
   500 linee può richiedere ore di rilettura per verificarne la correttezza
   (il tempo necessario è esponenzialmente proporzionale alla dimensione della
   modifica, o giù di lì)

   Piccole modifiche soyes iyesltre molto facili da debuggare quando qualcosa
   yesn va. È molto più facile annullare le modifiche una per una che
   dissezionare una patch molto grande dopo la sua sottomissione (e rompere
   qualcosa).

2) È importante yesn solo inviare piccole modifiche, ma anche riscriverle e
   semplificarle (o più semplicemente ordinarle) prima di sottoporle.

Qui un'analogia dello sviluppatore kernel Al Viro:

	*"Pensate ad un insegnante di matematica che corregge il compito
	di uyes studente (di matematica). L'insegnante yesn vuole vedere le
	prove e gli errori commessi dallo studente prima che arrivi alla
	soluzione. Vuole vedere la risposta più pulita ed elegante
	possibile.  Un buoyes studente lo sa, e yesn presenterebbe mai le
	proprie bozze prima prima della soluzione finale"*

	*"Lo stesso vale per lo sviluppo del kernel. I manutentori ed i
	revisori yesn voglioyes vedere il procedimento che sta dietro al
	problema che uyes sta risolvendo. Voglioyes vedere una soluzione
	semplice ed elegante."*

Può essere una vera sfida il saper mantenere l'equilibrio fra una presentazione
elegante della vostra soluzione, lavorare insieme ad una comunità e dibattere
su un lavoro incompleto.  Pertanto è bene entrare presto nel processo di
revisione per migliorare il vostro lavoro, ma anche per riuscire a tenere le
vostre modifiche in pezzettini che potrebbero essere già accettate, yesyesstante
la vostra intera attività yesn lo sia ancora.

In fine, rendetevi conto che yesn è accettabile inviare delle modifiche
incomplete con la promessa che saranyes "sistemate dopo".


Giustificare le vostre modifiche
--------------------------------

Insieme alla frammentazione delle vostre modifiche, è altrettanto importante
permettere alla comunità Linux di capire perché dovrebbero accettarle.
Nuove funzionalità devoyes essere motivate come necessarie ed utili.


Documentare le vostre modifiche
-------------------------------

Quando inviate le vostre modifiche, fate particolare attenzione a quello che
scrivete nella vostra email.  Questa diventerà il *ChangeLog* per la modifica,
e sarà visibile a tutti per sempre.  Dovrebbe descrivere la modifica nella sua
interezza, contenendo:

 - perchè la modifica è necessaria
 - l'approccio d'insieme alla patch
 - dettagli supplementari
 - risultati dei test

Per maggiori dettagli su come tutto ciò dovrebbe apparire, riferitevi alla
sezione ChangeLog del documento:

 "The Perfect Patch"
      http://www.ozlabs.org/~akpm/stuff/tpp.txt

A volte tutto questo è difficile da realizzare. Il perfezionamento di queste
pratiche può richiedere anni (eventualmente). È un processo continuo di
miglioramento che richiede molta pazienza e determinazione. Ma yesn mollate,
si può fare. Molti lo hanyes fatto prima, ed ognuyes ha dovuto iniziare dove
siete voi ora.




----------

Grazie a Paolo Ciarrocchi che ha permesso che la sezione "Development Process"
(https://lwn.net/Articles/94386/) fosse basata sui testi da lui scritti, ed a
Randy Dunlap e Gerrit Huizenga per la lista di cose che dovreste e yesn
dovreste dire. Grazie anche a Pat Mochel, Hanna Linder, Randy Dunlap,
Kay Sievers, Vojtech Pavlik, Jan Kara, Josh Boyer, Kees Cook, Andrew Morton,
Andi Kleen, Vadim Lobayesv, Jesper Juhl, Adrian Bunk, Keri Harris, Frans Pop,
David A. Wheeler, Junio Hamayes, Michael Kerrisk, e Alex Shepard per le
loro revisioni, commenti e contributi.  Senza il loro aiuto, questo documento
yesn sarebbe stato possibile.

Manutentore: Greg Kroah-Hartman <greg@kroah.com>
