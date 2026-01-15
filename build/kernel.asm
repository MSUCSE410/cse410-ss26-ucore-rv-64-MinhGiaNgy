
build/kernel:     file format elf64-littleriscv


Disassembly of section .text:

0000000080200000 <_entry>:
    .section .text.entry
    .globl _entry
_entry:
    la sp, boot_stack_top
    80200000:	00012117          	auipc	sp,0x12
    80200004:	00010113          	mv	sp,sp
    call main
    80200008:	06a000ef          	jal	ra,80200072 <main>

000000008020000c <consputc>:
#include "console.h"
#include "sbi.h"

void consputc(int c)
{
    8020000c:	1141                	addi	sp,sp,-16
    8020000e:	e406                	sd	ra,8(sp)
    80200010:	e022                	sd	s0,0(sp)
    80200012:	0800                	addi	s0,sp,16
	console_putchar(c);
    80200014:	00000097          	auipc	ra,0x0
    80200018:	472080e7          	jalr	1138(ra) # 80200486 <console_putchar>
}
    8020001c:	60a2                	ld	ra,8(sp)
    8020001e:	6402                	ld	s0,0(sp)
    80200020:	0141                	addi	sp,sp,16
    80200022:	8082                	ret

0000000080200024 <console_init>:

void console_init()
{
    80200024:	1141                	addi	sp,sp,-16
    80200026:	e422                	sd	s0,8(sp)
    80200028:	0800                	addi	s0,sp,16
	// DO NOTHING
    8020002a:	6422                	ld	s0,8(sp)
    8020002c:	0141                	addi	sp,sp,16
    8020002e:	8082                	ret

0000000080200030 <threadid>:
extern char e_data[];
extern char s_bss[];
extern char e_bss[];

int threadid()
{
    80200030:	1141                	addi	sp,sp,-16
    80200032:	e422                	sd	s0,8(sp)
    80200034:	0800                	addi	s0,sp,16
	return 0;
}
    80200036:	4501                	li	a0,0
    80200038:	6422                	ld	s0,8(sp)
    8020003a:	0141                	addi	sp,sp,16
    8020003c:	8082                	ret

000000008020003e <clean_bss>:

void clean_bss()
{
    8020003e:	1141                	addi	sp,sp,-16
    80200040:	e422                	sd	s0,8(sp)
    80200042:	0800                	addi	s0,sp,16
	char *p;
	for (p = s_bss; p < e_bss; ++p)
    80200044:	00012717          	auipc	a4,0x12
    80200048:	fbc70713          	addi	a4,a4,-68 # 80212000 <boot_stack_top>
    8020004c:	00012797          	auipc	a5,0x12
    80200050:	fb478793          	addi	a5,a5,-76 # 80212000 <boot_stack_top>
    80200054:	00f77c63          	bgeu	a4,a5,8020006c <clean_bss+0x2e>
    80200058:	87ba                	mv	a5,a4
    8020005a:	00012717          	auipc	a4,0x12
    8020005e:	fa670713          	addi	a4,a4,-90 # 80212000 <boot_stack_top>
		*p = 0;
    80200062:	00078023          	sb	zero,0(a5)
	for (p = s_bss; p < e_bss; ++p)
    80200066:	0785                	addi	a5,a5,1
    80200068:	fee79de3          	bne	a5,a4,80200062 <clean_bss+0x24>
}
    8020006c:	6422                	ld	s0,8(sp)
    8020006e:	0141                	addi	sp,sp,16
    80200070:	8082                	ret

0000000080200072 <main>:

void main()
{
    80200072:	1141                	addi	sp,sp,-16
    80200074:	e406                	sd	ra,8(sp)
    80200076:	e022                	sd	s0,0(sp)
    80200078:	0800                	addi	s0,sp,16
	clean_bss();
    8020007a:	00000097          	auipc	ra,0x0
    8020007e:	fc4080e7          	jalr	-60(ra) # 8020003e <clean_bss>
	console_init();
    80200082:	00000097          	auipc	ra,0x0
    80200086:	fa2080e7          	jalr	-94(ra) # 80200024 <console_init>
	printf("\n");
    8020008a:	00001517          	auipc	a0,0x1
    8020008e:	0de50513          	addi	a0,a0,222 # 80201168 <e_text+0x168>
    80200092:	00000097          	auipc	ra,0x0
    80200096:	21e080e7          	jalr	542(ra) # 802002b0 <printf>
	printf("hello wrold!\n");
    8020009a:	00001517          	auipc	a0,0x1
    8020009e:	f6650513          	addi	a0,a0,-154 # 80201000 <e_text>
    802000a2:	00000097          	auipc	ra,0x0
    802000a6:	20e080e7          	jalr	526(ra) # 802002b0 <printf>
	errorf("stext: %p", s_text);
    802000aa:	00000717          	auipc	a4,0x0
    802000ae:	f5670713          	addi	a4,a4,-170 # 80200000 <_entry>
    802000b2:	4681                	li	a3,0
    802000b4:	00001617          	auipc	a2,0x1
    802000b8:	f5c60613          	addi	a2,a2,-164 # 80201010 <e_text+0x10>
    802000bc:	45fd                	li	a1,31
    802000be:	00001517          	auipc	a0,0x1
    802000c2:	f5a50513          	addi	a0,a0,-166 # 80201018 <e_text+0x18>
    802000c6:	00000097          	auipc	ra,0x0
    802000ca:	1ea080e7          	jalr	490(ra) # 802002b0 <printf>
	warnf("etext: %p", e_text);
    802000ce:	00001717          	auipc	a4,0x1
    802000d2:	f3270713          	addi	a4,a4,-206 # 80201000 <e_text>
    802000d6:	4681                	li	a3,0
    802000d8:	00001617          	auipc	a2,0x1
    802000dc:	f6060613          	addi	a2,a2,-160 # 80201038 <e_text+0x38>
    802000e0:	05d00593          	li	a1,93
    802000e4:	00001517          	auipc	a0,0x1
    802000e8:	f5c50513          	addi	a0,a0,-164 # 80201040 <e_text+0x40>
    802000ec:	00000097          	auipc	ra,0x0
    802000f0:	1c4080e7          	jalr	452(ra) # 802002b0 <printf>
	infof("sroda: %p", s_rodata);
    802000f4:	00001717          	auipc	a4,0x1
    802000f8:	f0c70713          	addi	a4,a4,-244 # 80201000 <e_text>
    802000fc:	4681                	li	a3,0
    802000fe:	00001617          	auipc	a2,0x1
    80200102:	f6260613          	addi	a2,a2,-158 # 80201060 <e_text+0x60>
    80200106:	02200593          	li	a1,34
    8020010a:	00001517          	auipc	a0,0x1
    8020010e:	f5e50513          	addi	a0,a0,-162 # 80201068 <e_text+0x68>
    80200112:	00000097          	auipc	ra,0x0
    80200116:	19e080e7          	jalr	414(ra) # 802002b0 <printf>
	debugf("eroda: %p", e_rodata);
    8020011a:	00002717          	auipc	a4,0x2
    8020011e:	ee670713          	addi	a4,a4,-282 # 80202000 <boot_stack>
    80200122:	4681                	li	a3,0
    80200124:	00001617          	auipc	a2,0x1
    80200128:	f6460613          	addi	a2,a2,-156 # 80201088 <e_text+0x88>
    8020012c:	02000593          	li	a1,32
    80200130:	00001517          	auipc	a0,0x1
    80200134:	f6050513          	addi	a0,a0,-160 # 80201090 <e_text+0x90>
    80200138:	00000097          	auipc	ra,0x0
    8020013c:	178080e7          	jalr	376(ra) # 802002b0 <printf>
	debugf("sdata: %p", s_data);
    80200140:	00002717          	auipc	a4,0x2
    80200144:	ec070713          	addi	a4,a4,-320 # 80202000 <boot_stack>
    80200148:	4681                	li	a3,0
    8020014a:	00001617          	auipc	a2,0x1
    8020014e:	f3e60613          	addi	a2,a2,-194 # 80201088 <e_text+0x88>
    80200152:	02000593          	li	a1,32
    80200156:	00001517          	auipc	a0,0x1
    8020015a:	f5a50513          	addi	a0,a0,-166 # 802010b0 <e_text+0xb0>
    8020015e:	00000097          	auipc	ra,0x0
    80200162:	152080e7          	jalr	338(ra) # 802002b0 <printf>
	infof("edata: %p", e_data);
    80200166:	00002717          	auipc	a4,0x2
    8020016a:	e9a70713          	addi	a4,a4,-358 # 80202000 <boot_stack>
    8020016e:	4681                	li	a3,0
    80200170:	00001617          	auipc	a2,0x1
    80200174:	ef060613          	addi	a2,a2,-272 # 80201060 <e_text+0x60>
    80200178:	02200593          	li	a1,34
    8020017c:	00001517          	auipc	a0,0x1
    80200180:	f5450513          	addi	a0,a0,-172 # 802010d0 <e_text+0xd0>
    80200184:	00000097          	auipc	ra,0x0
    80200188:	12c080e7          	jalr	300(ra) # 802002b0 <printf>
	warnf("sbss : %p", s_bss);
    8020018c:	00012717          	auipc	a4,0x12
    80200190:	e7470713          	addi	a4,a4,-396 # 80212000 <boot_stack_top>
    80200194:	4681                	li	a3,0
    80200196:	00001617          	auipc	a2,0x1
    8020019a:	ea260613          	addi	a2,a2,-350 # 80201038 <e_text+0x38>
    8020019e:	05d00593          	li	a1,93
    802001a2:	00001517          	auipc	a0,0x1
    802001a6:	f4e50513          	addi	a0,a0,-178 # 802010f0 <e_text+0xf0>
    802001aa:	00000097          	auipc	ra,0x0
    802001ae:	106080e7          	jalr	262(ra) # 802002b0 <printf>
	errorf("ebss : %p", e_bss);
    802001b2:	00012717          	auipc	a4,0x12
    802001b6:	e4e70713          	addi	a4,a4,-434 # 80212000 <boot_stack_top>
    802001ba:	4681                	li	a3,0
    802001bc:	00001617          	auipc	a2,0x1
    802001c0:	e5460613          	addi	a2,a2,-428 # 80201010 <e_text+0x10>
    802001c4:	45fd                	li	a1,31
    802001c6:	00001517          	auipc	a0,0x1
    802001ca:	f4a50513          	addi	a0,a0,-182 # 80201110 <e_text+0x110>
    802001ce:	00000097          	auipc	ra,0x0
    802001d2:	0e2080e7          	jalr	226(ra) # 802002b0 <printf>
	panic("ALL DONE");
    802001d6:	02700793          	li	a5,39
    802001da:	00001717          	auipc	a4,0x1
    802001de:	f5670713          	addi	a4,a4,-170 # 80201130 <e_text+0x130>
    802001e2:	4681                	li	a3,0
    802001e4:	00001617          	auipc	a2,0x1
    802001e8:	f5c60613          	addi	a2,a2,-164 # 80201140 <e_text+0x140>
    802001ec:	45fd                	li	a1,31
    802001ee:	00001517          	auipc	a0,0x1
    802001f2:	f5a50513          	addi	a0,a0,-166 # 80201148 <e_text+0x148>
    802001f6:	00000097          	auipc	ra,0x0
    802001fa:	0ba080e7          	jalr	186(ra) # 802002b0 <printf>
    802001fe:	00000097          	auipc	ra,0x0
    80200202:	2b8080e7          	jalr	696(ra) # 802004b6 <shutdown>
}
    80200206:	60a2                	ld	ra,8(sp)
    80200208:	6402                	ld	s0,0(sp)
    8020020a:	0141                	addi	sp,sp,16
    8020020c:	8082                	ret

000000008020020e <printint>:
#include "console.h"
#include "defs.h"
static char digits[] = "0123456789abcdef";

static void printint(int xx, int base, int sign)
{
    8020020e:	7179                	addi	sp,sp,-48
    80200210:	f406                	sd	ra,40(sp)
    80200212:	f022                	sd	s0,32(sp)
    80200214:	ec26                	sd	s1,24(sp)
    80200216:	e84a                	sd	s2,16(sp)
    80200218:	1800                	addi	s0,sp,48
	char buf[16];
	int i;
	uint x;

	if (sign && (sign = xx < 0))
    8020021a:	c219                	beqz	a2,80200220 <printint+0x12>
    8020021c:	08054663          	bltz	a0,802002a8 <printint+0x9a>
		x = -xx;
	else
		x = xx;
    80200220:	2501                	sext.w	a0,a0
    80200222:	4881                	li	a7,0
    80200224:	fd040693          	addi	a3,s0,-48

	i = 0;
    80200228:	4701                	li	a4,0
	do {
		buf[i++] = digits[x % base];
    8020022a:	2581                	sext.w	a1,a1
    8020022c:	00001617          	auipc	a2,0x1
    80200230:	f8460613          	addi	a2,a2,-124 # 802011b0 <digits>
    80200234:	883a                	mv	a6,a4
    80200236:	2705                	addiw	a4,a4,1
    80200238:	02b577bb          	remuw	a5,a0,a1
    8020023c:	1782                	slli	a5,a5,0x20
    8020023e:	9381                	srli	a5,a5,0x20
    80200240:	97b2                	add	a5,a5,a2
    80200242:	0007c783          	lbu	a5,0(a5)
    80200246:	00f68023          	sb	a5,0(a3)
	} while ((x /= base) != 0);
    8020024a:	0005079b          	sext.w	a5,a0
    8020024e:	02b5553b          	divuw	a0,a0,a1
    80200252:	0685                	addi	a3,a3,1
    80200254:	feb7f0e3          	bgeu	a5,a1,80200234 <printint+0x26>

	if (sign)
    80200258:	00088b63          	beqz	a7,8020026e <printint+0x60>
		buf[i++] = '-';
    8020025c:	fe040793          	addi	a5,s0,-32
    80200260:	973e                	add	a4,a4,a5
    80200262:	02d00793          	li	a5,45
    80200266:	fef70823          	sb	a5,-16(a4)
    8020026a:	0028071b          	addiw	a4,a6,2

	while (--i >= 0)
    8020026e:	02e05763          	blez	a4,8020029c <printint+0x8e>
    80200272:	fd040793          	addi	a5,s0,-48
    80200276:	00e784b3          	add	s1,a5,a4
    8020027a:	fff78913          	addi	s2,a5,-1
    8020027e:	993a                	add	s2,s2,a4
    80200280:	377d                	addiw	a4,a4,-1
    80200282:	1702                	slli	a4,a4,0x20
    80200284:	9301                	srli	a4,a4,0x20
    80200286:	40e90933          	sub	s2,s2,a4
		consputc(buf[i]);
    8020028a:	fff4c503          	lbu	a0,-1(s1)
    8020028e:	00000097          	auipc	ra,0x0
    80200292:	d7e080e7          	jalr	-642(ra) # 8020000c <consputc>
	while (--i >= 0)
    80200296:	14fd                	addi	s1,s1,-1
    80200298:	ff2499e3          	bne	s1,s2,8020028a <printint+0x7c>
}
    8020029c:	70a2                	ld	ra,40(sp)
    8020029e:	7402                	ld	s0,32(sp)
    802002a0:	64e2                	ld	s1,24(sp)
    802002a2:	6942                	ld	s2,16(sp)
    802002a4:	6145                	addi	sp,sp,48
    802002a6:	8082                	ret
		x = -xx;
    802002a8:	40a0053b          	negw	a0,a0
	if (sign && (sign = xx < 0))
    802002ac:	4885                	li	a7,1
		x = -xx;
    802002ae:	bf9d                	j	80200224 <printint+0x16>

00000000802002b0 <printf>:
		consputc(digits[x >> (sizeof(uint64) * 8 - 4)]);
}

// Print to the console. only understands %d, %x, %p, %s.
void printf(char *fmt, ...)
{
    802002b0:	7131                	addi	sp,sp,-192
    802002b2:	fc86                	sd	ra,120(sp)
    802002b4:	f8a2                	sd	s0,112(sp)
    802002b6:	f4a6                	sd	s1,104(sp)
    802002b8:	f0ca                	sd	s2,96(sp)
    802002ba:	ecce                	sd	s3,88(sp)
    802002bc:	e8d2                	sd	s4,80(sp)
    802002be:	e4d6                	sd	s5,72(sp)
    802002c0:	e0da                	sd	s6,64(sp)
    802002c2:	fc5e                	sd	s7,56(sp)
    802002c4:	f862                	sd	s8,48(sp)
    802002c6:	f466                	sd	s9,40(sp)
    802002c8:	f06a                	sd	s10,32(sp)
    802002ca:	ec6e                	sd	s11,24(sp)
    802002cc:	0100                	addi	s0,sp,128
    802002ce:	8a2a                	mv	s4,a0
    802002d0:	e40c                	sd	a1,8(s0)
    802002d2:	e810                	sd	a2,16(s0)
    802002d4:	ec14                	sd	a3,24(s0)
    802002d6:	f018                	sd	a4,32(s0)
    802002d8:	f41c                	sd	a5,40(s0)
    802002da:	03043823          	sd	a6,48(s0)
    802002de:	03143c23          	sd	a7,56(s0)
	va_list ap;
	int i, c;
	char *s;

	if (fmt == 0)
    802002e2:	c915                	beqz	a0,80200316 <printf+0x66>
		panic("null fmt");

	va_start(ap, fmt);
    802002e4:	00840793          	addi	a5,s0,8
    802002e8:	f8f43423          	sd	a5,-120(s0)
	for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
    802002ec:	000a4503          	lbu	a0,0(s4)
    802002f0:	16050c63          	beqz	a0,80200468 <printf+0x1b8>
    802002f4:	4981                	li	s3,0
		if (c != '%') {
    802002f6:	02500a93          	li	s5,37
			continue;
		}
		c = fmt[++i] & 0xff;
		if (c == 0)
			break;
		switch (c) {
    802002fa:	07000b93          	li	s7,112
	consputc('x');
    802002fe:	4d41                	li	s10,16
		consputc(digits[x >> (sizeof(uint64) * 8 - 4)]);
    80200300:	00001b17          	auipc	s6,0x1
    80200304:	eb0b0b13          	addi	s6,s6,-336 # 802011b0 <digits>
		switch (c) {
    80200308:	07300c93          	li	s9,115
			printptr(va_arg(ap, uint64));
			break;
		case 's':
			if ((s = va_arg(ap, char *)) == 0)
				s = "(null)";
			for (; *s; s++)
    8020030c:	02800d93          	li	s11,40
		switch (c) {
    80200310:	06400c13          	li	s8,100
    80200314:	a889                	j	80200366 <printf+0xb6>
		panic("null fmt");
    80200316:	00000097          	auipc	ra,0x0
    8020031a:	d1a080e7          	jalr	-742(ra) # 80200030 <threadid>
    8020031e:	86aa                	mv	a3,a0
    80200320:	02e00793          	li	a5,46
    80200324:	00001717          	auipc	a4,0x1
    80200328:	e5470713          	addi	a4,a4,-428 # 80201178 <e_text+0x178>
    8020032c:	00001617          	auipc	a2,0x1
    80200330:	e1460613          	addi	a2,a2,-492 # 80201140 <e_text+0x140>
    80200334:	45fd                	li	a1,31
    80200336:	00001517          	auipc	a0,0x1
    8020033a:	e5250513          	addi	a0,a0,-430 # 80201188 <e_text+0x188>
    8020033e:	00000097          	auipc	ra,0x0
    80200342:	f72080e7          	jalr	-142(ra) # 802002b0 <printf>
    80200346:	00000097          	auipc	ra,0x0
    8020034a:	170080e7          	jalr	368(ra) # 802004b6 <shutdown>
    8020034e:	bf59                	j	802002e4 <printf+0x34>
			consputc(c);
    80200350:	00000097          	auipc	ra,0x0
    80200354:	cbc080e7          	jalr	-836(ra) # 8020000c <consputc>
	for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
    80200358:	2985                	addiw	s3,s3,1
    8020035a:	013a07b3          	add	a5,s4,s3
    8020035e:	0007c503          	lbu	a0,0(a5)
    80200362:	10050363          	beqz	a0,80200468 <printf+0x1b8>
		if (c != '%') {
    80200366:	ff5515e3          	bne	a0,s5,80200350 <printf+0xa0>
		c = fmt[++i] & 0xff;
    8020036a:	2985                	addiw	s3,s3,1
    8020036c:	013a07b3          	add	a5,s4,s3
    80200370:	0007c783          	lbu	a5,0(a5)
    80200374:	0007849b          	sext.w	s1,a5
		if (c == 0)
    80200378:	cbe5                	beqz	a5,80200468 <printf+0x1b8>
		switch (c) {
    8020037a:	05778a63          	beq	a5,s7,802003ce <printf+0x11e>
    8020037e:	02fbf663          	bgeu	s7,a5,802003aa <printf+0xfa>
    80200382:	09978863          	beq	a5,s9,80200412 <printf+0x162>
    80200386:	07800713          	li	a4,120
    8020038a:	0ce79463          	bne	a5,a4,80200452 <printf+0x1a2>
			printint(va_arg(ap, int), 16, 1);
    8020038e:	f8843783          	ld	a5,-120(s0)
    80200392:	00878713          	addi	a4,a5,8
    80200396:	f8e43423          	sd	a4,-120(s0)
    8020039a:	4605                	li	a2,1
    8020039c:	85ea                	mv	a1,s10
    8020039e:	4388                	lw	a0,0(a5)
    802003a0:	00000097          	auipc	ra,0x0
    802003a4:	e6e080e7          	jalr	-402(ra) # 8020020e <printint>
			break;
    802003a8:	bf45                	j	80200358 <printf+0xa8>
		switch (c) {
    802003aa:	09578e63          	beq	a5,s5,80200446 <printf+0x196>
    802003ae:	0b879263          	bne	a5,s8,80200452 <printf+0x1a2>
			printint(va_arg(ap, int), 10, 1);
    802003b2:	f8843783          	ld	a5,-120(s0)
    802003b6:	00878713          	addi	a4,a5,8
    802003ba:	f8e43423          	sd	a4,-120(s0)
    802003be:	4605                	li	a2,1
    802003c0:	45a9                	li	a1,10
    802003c2:	4388                	lw	a0,0(a5)
    802003c4:	00000097          	auipc	ra,0x0
    802003c8:	e4a080e7          	jalr	-438(ra) # 8020020e <printint>
			break;
    802003cc:	b771                	j	80200358 <printf+0xa8>
			printptr(va_arg(ap, uint64));
    802003ce:	f8843783          	ld	a5,-120(s0)
    802003d2:	00878713          	addi	a4,a5,8
    802003d6:	f8e43423          	sd	a4,-120(s0)
    802003da:	0007b903          	ld	s2,0(a5)
	consputc('0');
    802003de:	03000513          	li	a0,48
    802003e2:	00000097          	auipc	ra,0x0
    802003e6:	c2a080e7          	jalr	-982(ra) # 8020000c <consputc>
	consputc('x');
    802003ea:	07800513          	li	a0,120
    802003ee:	00000097          	auipc	ra,0x0
    802003f2:	c1e080e7          	jalr	-994(ra) # 8020000c <consputc>
    802003f6:	84ea                	mv	s1,s10
		consputc(digits[x >> (sizeof(uint64) * 8 - 4)]);
    802003f8:	03c95793          	srli	a5,s2,0x3c
    802003fc:	97da                	add	a5,a5,s6
    802003fe:	0007c503          	lbu	a0,0(a5)
    80200402:	00000097          	auipc	ra,0x0
    80200406:	c0a080e7          	jalr	-1014(ra) # 8020000c <consputc>
	for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
    8020040a:	0912                	slli	s2,s2,0x4
    8020040c:	34fd                	addiw	s1,s1,-1
    8020040e:	f4ed                	bnez	s1,802003f8 <printf+0x148>
    80200410:	b7a1                	j	80200358 <printf+0xa8>
			if ((s = va_arg(ap, char *)) == 0)
    80200412:	f8843783          	ld	a5,-120(s0)
    80200416:	00878713          	addi	a4,a5,8
    8020041a:	f8e43423          	sd	a4,-120(s0)
    8020041e:	6384                	ld	s1,0(a5)
    80200420:	cc89                	beqz	s1,8020043a <printf+0x18a>
			for (; *s; s++)
    80200422:	0004c503          	lbu	a0,0(s1)
    80200426:	d90d                	beqz	a0,80200358 <printf+0xa8>
				consputc(*s);
    80200428:	00000097          	auipc	ra,0x0
    8020042c:	be4080e7          	jalr	-1052(ra) # 8020000c <consputc>
			for (; *s; s++)
    80200430:	0485                	addi	s1,s1,1
    80200432:	0004c503          	lbu	a0,0(s1)
    80200436:	f96d                	bnez	a0,80200428 <printf+0x178>
    80200438:	b705                	j	80200358 <printf+0xa8>
				s = "(null)";
    8020043a:	00001497          	auipc	s1,0x1
    8020043e:	d3648493          	addi	s1,s1,-714 # 80201170 <e_text+0x170>
			for (; *s; s++)
    80200442:	856e                	mv	a0,s11
    80200444:	b7d5                	j	80200428 <printf+0x178>
			break;
		case '%':
			consputc('%');
    80200446:	8556                	mv	a0,s5
    80200448:	00000097          	auipc	ra,0x0
    8020044c:	bc4080e7          	jalr	-1084(ra) # 8020000c <consputc>
			break;
    80200450:	b721                	j	80200358 <printf+0xa8>
		default:
			// Print unknown % sequence to draw attention.
			consputc('%');
    80200452:	8556                	mv	a0,s5
    80200454:	00000097          	auipc	ra,0x0
    80200458:	bb8080e7          	jalr	-1096(ra) # 8020000c <consputc>
			consputc(c);
    8020045c:	8526                	mv	a0,s1
    8020045e:	00000097          	auipc	ra,0x0
    80200462:	bae080e7          	jalr	-1106(ra) # 8020000c <consputc>
			break;
    80200466:	bdcd                	j	80200358 <printf+0xa8>
		}
	}
    80200468:	70e6                	ld	ra,120(sp)
    8020046a:	7446                	ld	s0,112(sp)
    8020046c:	74a6                	ld	s1,104(sp)
    8020046e:	7906                	ld	s2,96(sp)
    80200470:	69e6                	ld	s3,88(sp)
    80200472:	6a46                	ld	s4,80(sp)
    80200474:	6aa6                	ld	s5,72(sp)
    80200476:	6b06                	ld	s6,64(sp)
    80200478:	7be2                	ld	s7,56(sp)
    8020047a:	7c42                	ld	s8,48(sp)
    8020047c:	7ca2                	ld	s9,40(sp)
    8020047e:	7d02                	ld	s10,32(sp)
    80200480:	6de2                	ld	s11,24(sp)
    80200482:	6129                	addi	sp,sp,192
    80200484:	8082                	ret

0000000080200486 <console_putchar>:
		     : "memory");
	return a0;
}

void console_putchar(int c)
{
    80200486:	1141                	addi	sp,sp,-16
    80200488:	e422                	sd	s0,8(sp)
    8020048a:	0800                	addi	s0,sp,16
	register uint64 a1 asm("a1") = arg1;
    8020048c:	4581                	li	a1,0
	register uint64 a2 asm("a2") = arg2;
    8020048e:	4601                	li	a2,0
	register uint64 a7 asm("a7") = which;
    80200490:	4885                	li	a7,1
	asm volatile("ecall"
    80200492:	00000073          	ecall
	sbi_call(SBI_CONSOLE_PUTCHAR, c, 0, 0);
}
    80200496:	6422                	ld	s0,8(sp)
    80200498:	0141                	addi	sp,sp,16
    8020049a:	8082                	ret

000000008020049c <console_getchar>:

int console_getchar()
{
    8020049c:	1141                	addi	sp,sp,-16
    8020049e:	e422                	sd	s0,8(sp)
    802004a0:	0800                	addi	s0,sp,16
	register uint64 a0 asm("a0") = arg0;
    802004a2:	4501                	li	a0,0
	register uint64 a1 asm("a1") = arg1;
    802004a4:	4581                	li	a1,0
	register uint64 a2 asm("a2") = arg2;
    802004a6:	4601                	li	a2,0
	register uint64 a7 asm("a7") = which;
    802004a8:	4889                	li	a7,2
	asm volatile("ecall"
    802004aa:	00000073          	ecall
	return sbi_call(SBI_CONSOLE_GETCHAR, 0, 0, 0);
}
    802004ae:	2501                	sext.w	a0,a0
    802004b0:	6422                	ld	s0,8(sp)
    802004b2:	0141                	addi	sp,sp,16
    802004b4:	8082                	ret

00000000802004b6 <shutdown>:

void shutdown()
{
    802004b6:	1141                	addi	sp,sp,-16
    802004b8:	e422                	sd	s0,8(sp)
    802004ba:	0800                	addi	s0,sp,16
	register uint64 a0 asm("a0") = arg0;
    802004bc:	4501                	li	a0,0
	register uint64 a1 asm("a1") = arg1;
    802004be:	4581                	li	a1,0
	register uint64 a2 asm("a2") = arg2;
    802004c0:	4601                	li	a2,0
	register uint64 a7 asm("a7") = which;
    802004c2:	48a1                	li	a7,8
	asm volatile("ecall"
    802004c4:	00000073          	ecall
	sbi_call(SBI_SHUTDOWN, 0, 0, 0);
    802004c8:	6422                	ld	s0,8(sp)
    802004ca:	0141                	addi	sp,sp,16
    802004cc:	8082                	ret
	...
