_DATA$00 segment para 'DATA'
	ALIGN 16
	PUBLIC run, loader_start, run_end

InstallResMon sizestr <InstallResMon >
UninstallResMon sizestr <UninstallResMon >
MAX_PATH equ (260+1)

Loader struc
	realcall			dq		?
	loadLibraryW		dq		?
	getProcAddress		dq		?
	freeLibrary			dq		?
	dll					dw		MAX_PATH dup(?)
	install				db		InstallResMon dup(?)
	uninstall			db		UninstallResMon dup(?)
Loader ends

run PROC
	OPTION PROLOGUE:NONE, EPILOGUE:NONE
stop:
	jmp short stop
	call start
loader_start LABEL PTR
	Loader<0,0,0,0,<0>,'InstallResMon','UninstallResMon'>
start:
	pop rcx
	push rdi
	push rsi
	sub rsp,28h
	mov rdi,rcx
	lea rcx,[rdi.Loader.dll]
	mov rax,[rdi.Loader.loadLibraryW]
	call rax
	or rax,rax
    jz short not_found
	mov rsi,rax
	mov rcx,rax
	mov rax,[rdi.Loader.getProcAddress]
	lea rdx,[rdi.Loader.install]
	call rax
	or rax,rax
    jz short not_found
	call rax
	mov rax,[rdi.Loader.realcall]
	call rax
	lea rdx,[rdi.Loader.uninstall]
	mov rcx,rsi
	mov rax,[rdi.Loader.getProcAddress]
	call rax
	or rax,rax
    jz short not_found
	call rax
	mov rax,[rdi.Loader.freeLibrary]
	mov rcx,rsi
	call rax
not_found:
	add rsp,28h
	pop rsi
	pop rdi
	ret
run_end LABEL PTR
run ENDP

_DATA$00 ENDS
	
END
