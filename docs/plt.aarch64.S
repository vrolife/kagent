	.text
	.file	"a.c"
	.globl	foo                             // -- Begin function foo
	.p2align	2
	.type	foo,@function
foo:                                    // @foo
	.cfi_startproc
// %bb.0:
	adrp	x8, _bar
	ldr	x0, [x8, :lo12:_bar]
	br	x0
.Lfunc_end0:
	.size	foo, .Lfunc_end0-foo
	.cfi_endproc
                                        // -- End function
	.type	_bar,@object                    // @_bar
	.bss
	.globl	_bar
	.p2align	3
_bar:
	.xword	0
	.size	_bar, 8

	.ident	"Android (8490178, based on r450784d) clang version 14.0.6 (https://android.googlesource.com/toolchain/llvm-project 4c603efb0cca074e9238af8b4106c30add4418f6)"
	.section	".note.GNU-stack","",@progbits
