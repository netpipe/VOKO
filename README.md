# VOKO
![ScreenShot](./ss.png)<br>
Token system with batch verification for added security, can function as IOU's , vouchers or digital notes or weighted votes even.<br>
<br>
reopen application or wait 50000 ms to redeem expired token files.<br>
<br>
to use , generate all tokens button then select valid tokens ( basically grabs random tokens from initial generated ones) , after that you dont have to use those buttons unless generating a new one. exporting tokens and importing them after that with the buttons.<br>
<br>
non return makes it so you cant reissue the same numbers again / only works for making the exported files (spendable bills).<br>
<br>
./VOKO --generate-all --genAmount 800 --headless<br>
Generated/Selected all tokens.<br>
./VOKO --export 1 --headless<br>
"KIMNWZP1YZR7UDICQ8FYG35IA"<br>
./VOKO --redeem KIMNWZP1YZR7UDICQ8FYG35IA --headless --nonreturn<br>
"Token successfully redeemed."<br>


