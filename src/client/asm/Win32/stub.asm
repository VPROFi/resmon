.586
.model flat

_DATA$00 segment para 'DATA'
	ALIGN 4
	PUBLIC _run, _loader_start, _run_end

InstallResMon sizestr <InstallResMon >
UninstallResMon sizestr <UninstallResMon >
MAX_PATH equ (260+1)

Loader struc
	realcall			dd		?
	loadLibraryW		dd		?
	getProcAddress		dd		?
	freeLibrary			dd		?
	dll					dw		MAX_PATH dup(?)
	install				db		InstallResMon dup(?)
	uninstall			db		UninstallResMon dup(?)
Loader ends

_run PROC
	OPTION PROLOGUE:NONE, EPILOGUE:NONE
stop:
	jmp short stop
	call start
_loader_start LABEL PTR
	Loader<0,0,0,0,<0>,'InstallResMon','UninstallResMon'>
start:
	pop ecx
	push edi
	push esi
	mov edi,ecx
	lea ecx,[edi.Loader.dll]
	mov eax,[edi.Loader.loadLibraryW]
	push ecx
	call eax
	or eax,eax
    jz short not_found
	mov esi,eax
	mov ecx,eax
	mov eax,[edi.Loader.getProcAddress]
	lea edx,[edi.Loader.install]
	push edx
	push ecx
	call eax
	or eax,eax
    jz short not_found
	call eax
	mov eax,[edi.Loader.realcall]
	call eax
	lea edx,[edi.Loader.uninstall]
	mov ecx,esi
	mov eax,[edi.Loader.getProcAddress]
	push edx
	push ecx
	call eax
	or eax,eax
    jz short not_found
	call eax
	mov eax,[edi.Loader.freeLibrary]
	mov ecx,esi
	push ecx
	call eax
not_found:
	pop esi
	pop edi
	ret
_run_end LABEL PTR
_run ENDP

_DATA$00 ENDS
	
END
