
smp-x86_64-trampoline.o:     file format elf64-x86-64


Disassembly of section .rodata:

0000000000000000 <ihk_smp_trampoline_data>:
       0:	eb 26                	jmp    28 <cpu_start_body>
       2:	00 00                	add    BYTE PTR [rax],al
       4:	00 00                	add    BYTE PTR [rax],al
	...

0000000000000008 <header_pgtbl>:
	...

0000000000000010 <header_load>:
	...

0000000000000018 <stack_ptr>:
	...

0000000000000020 <notify_address>:
	...

0000000000000028 <cpu_start_body>:
      28:	fa                   	cli    
      29:	0f 09                	wbinvd 
      2b:	8c c8                	mov    eax,cs
      2d:	8e d8                	mov    ds,eax
      2f:	8e c0                	mov    es,eax
      31:	8e d0                	mov    ss,eax
      33:	66 31 db             	xor    bx,bx
      36:	8c cb                	mov    ebx,cs
      38:	66 c1 e3 04          	shl    bx,0x4
      3c:	66 01 1e             	add    WORD PTR [rsi],bx
      3f:	06                   	(bad)  
      40:	01 66 01             	add    DWORD PTR [rsi+0x1],esp
      43:	1e                   	(bad)  
      44:	67 00 66 01          	add    BYTE PTR [esi+0x1],ah
      48:	1e                   	(bad)  
      49:	3c 01                	cmp    al,0x1
      4b:	66 0f 01 16          	data16 lgdt [rsi]
      4f:	04 01                	add    al,0x1
      51:	66 0f 01 1e          	data16 lidt [rsi]
      55:	fe 00                	inc    BYTE PTR [rax]
      57:	eb 00                	jmp    59 <cpu_start_body+0x31>
      59:	0f 20 c2             	mov    rdx,cr0
      5c:	80 ca 01             	or     dl,0x1
      5f:	0f 22 c2             	mov    cr0,rdx
      // fails at the longjump ...
      62:	66 ff 2e             	jmp    DWORD PTR [rsi]
      65:	67 00 70 00          	add    BYTE PTR [eax+0x0],dh
      69:	00 00                	add    BYTE PTR [rax],al
      6b:	10 00                	adc    BYTE PTR [rax],al
      6d:	00 00                	add    BYTE PTR [rax],al
	...

0000000000000070 <protect_start>:
      70:	b8 18 00 00 00       	mov    eax,0x18
      75:	8e d8                	mov    ds,eax
      77:	8e d0                	mov    ss,eax
      79:	0f 20 e0             	mov    rax,cr4
      7c:	83 c8 20             	or     eax,0x20
      7f:	0f 22 e0             	mov    cr4,rax
      82:	8d a3 00 10 00 00    	lea    esp,[rbx+0x1000]
      88:	8b 83 08 00 00 00    	mov    eax,DWORD PTR [rbx+0x8]
      8e:	0f 22 d8             	mov    cr3,rax
      91:	8b 8b 10 00 00 00    	mov    ecx,DWORD PTR [rbx+0x10]
      97:	8b 83 18 00 00 00    	mov    eax,DWORD PTR [rbx+0x18]
      9d:	74 02                	je     a1 <protect_start+0x31>
      9f:	89 c4                	mov    esp,eax
      a1:	b9 80 00 00 c0       	mov    ecx,0xc0000080
      a6:	b8 00 01 00 00       	mov    eax,0x100
      ab:	31 d2                	xor    edx,edx
      ad:	0f 30                	wrmsr  
      af:	0f 20 c2             	mov    rdx,cr0
      b2:	81 ca 00 00 00 80    	or     edx,0x80000000
      b8:	0f 22 c2             	mov    cr0,rdx
      bb:	ff ab 3c 01 00 00    	jmp    FWORD PTR [rbx+0x13c]
      c1:	00 00                	add    BYTE PTR [rax],al
      c3:	00 00                	add    BYTE PTR [rax],al
      c5:	00 00                	add    BYTE PTR [rax],al
	...

00000000000000c8 <start_64>:
      c8:	0f 20 d8             	mov    rax,cr3
      cb:	0f 22 d8             	mov    cr3,rax
      ce:	67 48 8b 8b 10 00 00 00 	mov    rcx,QWORD PTR [ebx+0x10]
      d6:	48 83 f9 00          	cmp    rcx,0x0
      da:	74 17                	je     f3 <start_64+0x2b>
      dc:	67 48 8b 83 18 00 00 00 	mov    rax,QWORD PTR [ebx+0x18]
      e4:	74 03                	je     e9 <start_64+0x21>
      e6:	48 89 c4             	mov    rsp,rax
      e9:	67 48 8b bb 20 00 00 00 	mov    rdi,QWORD PTR [ebx+0x20]
      f1:	ff e1                	jmp    rcx
      f3:	48 c7 00 02 00 00 00 	mov    QWORD PTR [rax],0x2
      fa:	fa                   	cli    
      fb:	f4                   	hlt    
      fc:	eb fc                	jmp    fa <start_64+0x32>

00000000000000fe <boot_idtptr>:
      fe:	00 00                	add    BYTE PTR [rax],al
     100:	00 00                	add    BYTE PTR [rax],al
	...

0000000000000104 <boot_gdtptr>:
     104:	30 00                	xor    BYTE PTR [rax],al
     106:	0c 01                	or     al,0x1
     108:	00 00                	add    BYTE PTR [rax],al
	...

000000000000010c <boot_gdt32>:
	...
     11c:	ff                   	(bad)  
     11d:	ff 00                	inc    DWORD PTR [rax]
     11f:	00 00                	add    BYTE PTR [rax],al
     121:	9b                   	fwait
     122:	cf                   	iret   
     123:	00 ff                	add    bh,bh
     125:	ff 00                	inc    DWORD PTR [rax]
     127:	00 00                	add    BYTE PTR [rax],al
     129:	93                   	xchg   ebx,eax
     12a:	cf                   	iret   
     12b:	00 ff                	add    bh,bh
     12d:	ff 00                	inc    DWORD PTR [rax]
     12f:	00 00                	add    BYTE PTR [rax],al
     131:	9b                   	fwait
     132:	af                   	scas   eax,DWORD PTR es:[rdi]
     133:	00 67 00             	add    BYTE PTR [rdi+0x0],ah
     136:	00 00                	add    BYTE PTR [rax],al
     138:	00 89 00 00 c8 00    	add    BYTE PTR [rcx+0xc80000],cl

000000000000013c <boot_gdt32_end>:
     13c:	c8 00 00 00          	enter  0x0,0x0
     140:	20 00                	and    BYTE PTR [rax],al
	...

0000000000000144 <stack>:
	...

Disassembly of section .debug_line:

0000000000000000 <.debug_line>:
   0:	ce                   	(bad)  
   1:	00 00                	add    BYTE PTR [rax],al
   3:	00 02                	add    BYTE PTR [rdx],al
   5:	00 7a 00             	add    BYTE PTR [rdx+0x0],bh
   8:	00 00                	add    BYTE PTR [rax],al
   a:	01 01                	add    DWORD PTR [rcx],eax
   c:	fb                   	sti    
   d:	0e                   	(bad)  
   e:	0d 00 01 01 01       	or     eax,0x1010100
  13:	01 00                	add    DWORD PTR [rax],eax
  15:	00 00                	add    BYTE PTR [rax],al
  17:	01 00                	add    DWORD PTR [rax],eax
  19:	00 01                	add    BYTE PTR [rcx],al
  1b:	2f                   	(bad)  
  1c:	68 6f 6d 65 2f       	push   0x2f656d6f
  21:	72 6b                	jb     8e <.debug_line+0x8e>
  23:	75 62                	jne    87 <.debug_line+0x87>
  25:	61                   	(bad)  
  26:	6e                   	outs   dx,BYTE PTR ds:[rsi]
  27:	2f                   	(bad)  
  28:	6d                   	ins    DWORD PTR es:[rdi],dx
  29:	79 74                	jns    9f <.debug_line+0x9f>
  2b:	68 6f 73 2f 6d       	push   0x6d2f736f
  30:	79 74                	jns    a6 <.debug_line+0xa6>
  32:	68 6f 73 2d 70       	push   0x702d736f
  37:	75 62                	jne    9b <.debug_line+0x9b>
  39:	6c                   	ins    BYTE PTR es:[rdi],dx
  3a:	69 63 2f 33 72 64 70 	imul   esp,DWORD PTR [rbx+0x2f],0x70647233
  41:	61                   	(bad)  
  42:	72 74                	jb     b8 <.debug_line+0xb8>
  44:	79 2f                	jns    75 <.debug_line+0x75>
  46:	69 68 6b 2f 6c 69 6e 	imul   ebp,DWORD PTR [rax+0x6b],0x6e696c2f
  4d:	75 78                	jne    c7 <.debug_line+0xc7>
  4f:	2f                   	(bad)  
  50:	64 72 69             	fs jb  bc <.debug_line+0xbc>
  53:	76 65                	jbe    ba <.debug_line+0xba>
  55:	72 2f                	jb     86 <.debug_line+0x86>
  57:	73 6d                	jae    c6 <.debug_line+0xc6>
  59:	70 2f                	jo     8a <.debug_line+0x8a>
  5b:	61                   	(bad)  
  5c:	72 63                	jb     c1 <.debug_line+0xc1>
  5e:	68 2f 78 38 36       	push   0x3638782f
  63:	5f                   	pop    rdi
  64:	36 34 00             	ss xor al,0x0
  67:	00 73 6d             	add    BYTE PTR [rbx+0x6d],dh
  6a:	70 2d                	jo     99 <.debug_line+0x99>
  6c:	78 38                	js     a6 <.debug_line+0xa6>
  6e:	36 5f                	ss pop rdi
  70:	36 34 2d             	ss xor al,0x2d
  73:	74 72                	je     e7 <start_64+0x1f>
  75:	61                   	(bad)  
  76:	6d                   	ins    DWORD PTR es:[rdi],dx
  77:	70 6f                	jo     e8 <start_64+0x20>
  79:	6c                   	ins    BYTE PTR es:[rdi],dx
  7a:	69 6e 65 2e 53 00 01 	imul   ebp,DWORD PTR [rsi+0x65],0x100532e
  81:	00 00                	add    BYTE PTR [rax],al
  83:	00 00                	add    BYTE PTR [rax],al
  85:	09 02                	or     DWORD PTR [rdx],eax
	...
	87: R_X86_64_64	.rodata
  8f:	03 19                	add    ebx,DWORD PTR [rcx]
  91:	01 03                	add    DWORD PTR [rbx],eax
  93:	0d 02 28 01 21       	or     eax,0x21012802
  98:	30 2f                	xor    BYTE PTR [rdi],ch
  9a:	2f                   	(bad)  
  9b:	2f                   	(bad)  
  9c:	30 3d 2f 4f 59 59    	xor    BYTE PTR [rip+0x59594f2f],bh        # 59594fd1 <ihk_smp_trampoline_end+0x59593fd1>
  a2:	5a                   	pop    rdx
  a3:	67 68 31 3d 3d 3e    	addr32 push 0x3e3d3d31
  a9:	dd 59 2f             	fstp   QWORD PTR [rcx+0x2f]
  ac:	31 3d 3d 3e 69 67    	xor    DWORD PTR [rip+0x67693e3d],edi        # 67693eef <ihk_smp_trampoline_end+0x67692eef>
  b2:	3e 67 67 2f          	ds addr32 addr32 (bad) 
  b6:	32 59 59             	xor    bl,BYTE PTR [rcx+0x59]
  b9:	2f                   	(bad)  
  ba:	31 3d 67 3e ce 3d    	xor    DWORD PTR [rip+0x3dce3e67],edi        # 3dce3f27 <ihk_smp_trampoline_end+0x3dce2f27>
  c0:	3e 83 4c 30 83 2f    	or     DWORD PTR ds:[rax+rsi*1-0x7d],0x2f
  c6:	3f                   	(bad)  
  c7:	83 31 76             	xor    DWORD PTR [rcx],0x76
  ca:	21 21                	and    DWORD PTR [rcx],esp
  cc:	02                   	.byte 0x2
  cd:	84 1e                	test   BYTE PTR [rsi],bl
  cf:	00 01                	add    BYTE PTR [rcx],al
  d1:	01                   	.byte 0x1

Disassembly of section .debug_info:

0000000000000000 <.debug_info>:
   0:	b5 00                	mov    ch,0x0
   2:	00 00                	add    BYTE PTR [rax],al
   4:	02 00                	add    al,BYTE PTR [rax]
   6:	00 00                	add    BYTE PTR [rax],al	6: R_X86_64_32	.debug_abbrev
   8:	00 00                	add    BYTE PTR [rax],al
   a:	08 01                	or     BYTE PTR [rcx],al
	...
	c: R_X86_64_32	.debug_line
	10: R_X86_64_64	.rodata
	18: R_X86_64_64	.rodata+0x1000
  20:	2f                   	(bad)  
  21:	68 6f 6d 65 2f       	push   0x2f656d6f
  26:	72 6b                	jb     93 <.debug_info+0x93>
  28:	75 62                	jne    8c <.debug_info+0x8c>
  2a:	61                   	(bad)  
  2b:	6e                   	outs   dx,BYTE PTR ds:[rsi]
  2c:	2f                   	(bad)  
  2d:	6d                   	ins    DWORD PTR es:[rdi],dx
  2e:	79 74                	jns    a4 <.debug_info+0xa4>
  30:	68 6f 73 2f 6d       	push   0x6d2f736f
  35:	79 74                	jns    ab <.debug_info+0xab>
  37:	68 6f 73 2d 70       	push   0x702d736f
  3c:	75 62                	jne    a0 <.debug_info+0xa0>
  3e:	6c                   	ins    BYTE PTR es:[rdi],dx
  3f:	69 63 2f 33 72 64 70 	imul   esp,DWORD PTR [rbx+0x2f],0x70647233
  46:	61                   	(bad)  
  47:	72 74                	jb     bd <protect_start+0x4d>
  49:	79 2f                	jns    7a <.debug_info+0x7a>
  4b:	69 68 6b 2f 6c 69 6e 	imul   ebp,DWORD PTR [rax+0x6b],0x6e696c2f
  52:	75 78                	jne    cc <start_64+0x4>
  54:	2f                   	(bad)  
  55:	64 72 69             	fs jb  c1 <protect_start+0x51>
  58:	76 65                	jbe    bf <protect_start+0x4f>
  5a:	72 2f                	jb     8b <.debug_info+0x8b>
  5c:	73 6d                	jae    cb <start_64+0x3>
  5e:	70 2f                	jo     8f <.debug_info+0x8f>
  60:	61                   	(bad)  
  61:	72 63                	jb     c6 <protect_start+0x56>
  63:	68 2f 78 38 36       	push   0x3638782f
  68:	5f                   	pop    rdi
  69:	36 34 2f             	ss xor al,0x2f
  6c:	73 6d                	jae    db <start_64+0x13>
  6e:	70 2d                	jo     9d <.debug_info+0x9d>
  70:	78 38                	js     aa <.debug_info+0xaa>
  72:	36 5f                	ss pop rdi
  74:	36 34 2d             	ss xor al,0x2d
  77:	74 72                	je     eb <start_64+0x23>
  79:	61                   	(bad)  
  7a:	6d                   	ins    DWORD PTR es:[rdi],dx
  7b:	70 6f                	jo     ec <start_64+0x24>
  7d:	6c                   	ins    BYTE PTR es:[rdi],dx
  7e:	69 6e 65 2e 53 00 2f 	imul   ebp,DWORD PTR [rsi+0x65],0x2f00532e
  85:	75 73                	jne    fa <start_64+0x32>
  87:	72 2f                	jb     b8 <.debug_info+0xb8>
  89:	73 72                	jae    fd <start_64+0x35>
  8b:	63 2f                	movsxd ebp,DWORD PTR [rdi]
  8d:	6b 65 72 6e          	imul   esp,DWORD PTR [rbp+0x72],0x6e
  91:	65 6c                	gs ins BYTE PTR es:[rdi],dx
  93:	73 2f                	jae    c4 <protect_start+0x54>
  95:	33 2e                	xor    ebp,DWORD PTR [rsi]
  97:	31 30                	xor    DWORD PTR [rax],esi
  99:	2e 30 2d 35 31 34 2e 	xor    BYTE PTR cs:[rip+0x2e343135],ch        # 2e3431d5 <ihk_smp_trampoline_end+0x2e3421d5>
  a0:	65 6c                	gs ins BYTE PTR es:[rdi],dx
  a2:	37                   	(bad)  
  a3:	2e 78 38             	cs js  de <start_64+0x16>
  a6:	36 5f                	ss pop rdi
  a8:	36 34 00             	ss xor al,0x0
  ab:	47                   	rex.RXB
  ac:	4e 55                	rex.WRX push rbp
  ae:	20 41 53             	and    BYTE PTR [rcx+0x53],al
  b1:	20 32                	and    BYTE PTR [rdx],dh
  b3:	2e 32 37             	xor    dh,BYTE PTR cs:[rdi]
  b6:	00 01                	add    BYTE PTR [rcx],al
  b8:	80                   	.byte 0x80

Disassembly of section .debug_abbrev:

0000000000000000 <.debug_abbrev>:
   0:	01 11                	add    DWORD PTR [rcx],edx
   2:	00 10                	add    BYTE PTR [rax],dl
   4:	06                   	(bad)  
   5:	11 01                	adc    DWORD PTR [rcx],eax
   7:	12 01                	adc    al,BYTE PTR [rcx]
   9:	03 08                	add    ecx,DWORD PTR [rax]
   b:	1b 08                	sbb    ecx,DWORD PTR [rax]
   d:	25 08 13 05 00       	and    eax,0x51308
	...

Disassembly of section .debug_aranges:

0000000000000000 <.debug_aranges>:
   0:	2c 00                	sub    al,0x0
   2:	00 00                	add    BYTE PTR [rax],al
   4:	02 00                	add    al,BYTE PTR [rax]
   6:	00 00                	add    BYTE PTR [rax],al	6: R_X86_64_32	.debug_info
   8:	00 00                	add    BYTE PTR [rax],al
   a:	08 00                	or     BYTE PTR [rax],al
	...
	10: R_X86_64_64	.rodata
  18:	00 10                	add    BYTE PTR [rax],dl
	...
