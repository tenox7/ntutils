@echo off
::
:: Diff template for comparing TDE updates.
::
::       Function Prototype
:: -F "^[[:alnum:]_]\+  \?[[:alnum:]_]\+("
::
::               Menu     Variable      Function
diff.exe -u -F "^Begin(" -F "= {$" -F "^ \* Name:" %1 %2 %3 %4 %5 %6 %7 %8 %9
