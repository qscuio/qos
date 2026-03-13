
kernel:     file format elf32-i386


Disassembly of section .text:

80100000 <multiboot_header>:
80100000:	02 b0 ad 1b 00 00    	add    0x1bad(%eax),%dh
80100006:	00 00                	add    %al,(%eax)
80100008:	fe 4f 52             	decb   0x52(%edi)
8010000b:	e4                   	.byte 0xe4

8010000c <entry>:

# Entering xv6 on boot processor, with paging off.
.globl entry
entry:
  # Turn on page size extension for 4Mbyte pages
  movl    %cr4, %eax
8010000c:	0f 20 e0             	mov    %cr4,%eax
  orl     $(CR4_PSE), %eax
8010000f:	83 c8 10             	or     $0x10,%eax
  movl    %eax, %cr4
80100012:	0f 22 e0             	mov    %eax,%cr4
  # Set page directory
  movl    $(V2P_WO(entrypgdir)), %eax
80100015:	b8 00 90 10 00       	mov    $0x109000,%eax
  movl    %eax, %cr3
8010001a:	0f 22 d8             	mov    %eax,%cr3
  # Turn on paging.
  movl    %cr0, %eax
8010001d:	0f 20 c0             	mov    %cr0,%eax
  orl     $(CR0_PG|CR0_WP), %eax
80100020:	0d 00 00 01 80       	or     $0x80010000,%eax
  movl    %eax, %cr0
80100025:	0f 22 c0             	mov    %eax,%cr0

  # Set up the stack pointer.
  movl $(stack + KSTACKSIZE), %esp
80100028:	bc d0 54 11 80       	mov    $0x801154d0,%esp

  # Jump to main(), and switch to executing at
  # high addresses. The indirect call is needed because
  # the assembler produces a PC-relative instruction
  # for a direct jump.
  mov $main, %eax
8010002d:	b8 30 30 10 80       	mov    $0x80103030,%eax
  jmp *%eax
80100032:	ff e0                	jmp    *%eax
80100034:	66 90                	xchg   %ax,%ax
80100036:	66 90                	xchg   %ax,%ax
80100038:	66 90                	xchg   %ax,%ax
8010003a:	66 90                	xchg   %ax,%ax
8010003c:	66 90                	xchg   %ax,%ax
8010003e:	66 90                	xchg   %ax,%ax

80100040 <binit>:
  struct buf head;
} bcache;

void
binit(void)
{
80100040:	55                   	push   %ebp
80100041:	89 e5                	mov    %esp,%ebp
80100043:	53                   	push   %ebx

//PAGEBREAK!
  // Create linked list of buffers
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
80100044:	bb 54 a5 10 80       	mov    $0x8010a554,%ebx
{
80100049:	83 ec 0c             	sub    $0xc,%esp
  initlock(&bcache.lock, "bcache");
8010004c:	68 60 71 10 80       	push   $0x80107160
80100051:	68 20 a5 10 80       	push   $0x8010a520
80100056:	e8 55 43 00 00       	call   801043b0 <initlock>
  bcache.head.next = &bcache.head;
8010005b:	83 c4 10             	add    $0x10,%esp
8010005e:	b8 1c ec 10 80       	mov    $0x8010ec1c,%eax
  bcache.head.prev = &bcache.head;
80100063:	c7 05 6c ec 10 80 1c 	movl   $0x8010ec1c,0x8010ec6c
8010006a:	ec 10 80 
  bcache.head.next = &bcache.head;
8010006d:	c7 05 70 ec 10 80 1c 	movl   $0x8010ec1c,0x8010ec70
80100074:	ec 10 80 
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
80100077:	eb 09                	jmp    80100082 <binit+0x42>
80100079:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
80100080:	89 d3                	mov    %edx,%ebx
    b->next = bcache.head.next;
80100082:	89 43 54             	mov    %eax,0x54(%ebx)
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
80100085:	83 ec 08             	sub    $0x8,%esp
80100088:	8d 43 0c             	lea    0xc(%ebx),%eax
    b->prev = &bcache.head;
8010008b:	c7 43 50 1c ec 10 80 	movl   $0x8010ec1c,0x50(%ebx)
    initsleeplock(&b->lock, "buffer");
80100092:	68 67 71 10 80       	push   $0x80107167
80100097:	50                   	push   %eax
80100098:	e8 e3 41 00 00       	call   80104280 <initsleeplock>
    bcache.head.next->prev = b;
8010009d:	a1 70 ec 10 80       	mov    0x8010ec70,%eax
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
801000a2:	8d 93 5c 02 00 00    	lea    0x25c(%ebx),%edx
801000a8:	83 c4 10             	add    $0x10,%esp
    bcache.head.next->prev = b;
801000ab:	89 58 50             	mov    %ebx,0x50(%eax)
    bcache.head.next = b;
801000ae:	89 d8                	mov    %ebx,%eax
801000b0:	89 1d 70 ec 10 80    	mov    %ebx,0x8010ec70
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
801000b6:	81 fb c0 e9 10 80    	cmp    $0x8010e9c0,%ebx
801000bc:	75 c2                	jne    80100080 <binit+0x40>
  }
}
801000be:	8b 5d fc             	mov    -0x4(%ebp),%ebx
801000c1:	c9                   	leave
801000c2:	c3                   	ret
801000c3:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801000ca:	00 
801000cb:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi

801000d0 <bread>:
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
801000d0:	55                   	push   %ebp
801000d1:	89 e5                	mov    %esp,%ebp
801000d3:	57                   	push   %edi
801000d4:	56                   	push   %esi
801000d5:	53                   	push   %ebx
801000d6:	83 ec 18             	sub    $0x18,%esp
801000d9:	8b 75 08             	mov    0x8(%ebp),%esi
801000dc:	8b 7d 0c             	mov    0xc(%ebp),%edi
  acquire(&bcache.lock);
801000df:	68 20 a5 10 80       	push   $0x8010a520
801000e4:	e8 b7 44 00 00       	call   801045a0 <acquire>
  for(b = bcache.head.next; b != &bcache.head; b = b->next){
801000e9:	8b 1d 70 ec 10 80    	mov    0x8010ec70,%ebx
801000ef:	83 c4 10             	add    $0x10,%esp
801000f2:	81 fb 1c ec 10 80    	cmp    $0x8010ec1c,%ebx
801000f8:	75 11                	jne    8010010b <bread+0x3b>
801000fa:	eb 24                	jmp    80100120 <bread+0x50>
801000fc:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
80100100:	8b 5b 54             	mov    0x54(%ebx),%ebx
80100103:	81 fb 1c ec 10 80    	cmp    $0x8010ec1c,%ebx
80100109:	74 15                	je     80100120 <bread+0x50>
    if(b->dev == dev && b->blockno == blockno){
8010010b:	3b 73 04             	cmp    0x4(%ebx),%esi
8010010e:	75 f0                	jne    80100100 <bread+0x30>
80100110:	3b 7b 08             	cmp    0x8(%ebx),%edi
80100113:	75 eb                	jne    80100100 <bread+0x30>
      b->refcnt++;
80100115:	83 43 4c 01          	addl   $0x1,0x4c(%ebx)
      release(&bcache.lock);
80100119:	eb 3f                	jmp    8010015a <bread+0x8a>
8010011b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
80100120:	8b 1d 6c ec 10 80    	mov    0x8010ec6c,%ebx
80100126:	81 fb 1c ec 10 80    	cmp    $0x8010ec1c,%ebx
8010012c:	75 0d                	jne    8010013b <bread+0x6b>
8010012e:	eb 6e                	jmp    8010019e <bread+0xce>
80100130:	8b 5b 50             	mov    0x50(%ebx),%ebx
80100133:	81 fb 1c ec 10 80    	cmp    $0x8010ec1c,%ebx
80100139:	74 63                	je     8010019e <bread+0xce>
    if(b->refcnt == 0 && (b->flags & B_DIRTY) == 0) {
8010013b:	8b 43 4c             	mov    0x4c(%ebx),%eax
8010013e:	85 c0                	test   %eax,%eax
80100140:	75 ee                	jne    80100130 <bread+0x60>
80100142:	f6 03 04             	testb  $0x4,(%ebx)
80100145:	75 e9                	jne    80100130 <bread+0x60>
      b->dev = dev;
80100147:	89 73 04             	mov    %esi,0x4(%ebx)
      b->blockno = blockno;
8010014a:	89 7b 08             	mov    %edi,0x8(%ebx)
      b->flags = 0;
8010014d:	c7 03 00 00 00 00    	movl   $0x0,(%ebx)
      b->refcnt = 1;
80100153:	c7 43 4c 01 00 00 00 	movl   $0x1,0x4c(%ebx)
      release(&bcache.lock);
8010015a:	83 ec 0c             	sub    $0xc,%esp
8010015d:	68 20 a5 10 80       	push   $0x8010a520
80100162:	e8 d9 43 00 00       	call   80104540 <release>
      acquiresleep(&b->lock);
80100167:	8d 43 0c             	lea    0xc(%ebx),%eax
8010016a:	89 04 24             	mov    %eax,(%esp)
8010016d:	e8 4e 41 00 00       	call   801042c0 <acquiresleep>
      return b;
80100172:	83 c4 10             	add    $0x10,%esp
  struct buf *b;

  b = bget(dev, blockno);
  if((b->flags & B_VALID) == 0) {
80100175:	f6 03 02             	testb  $0x2,(%ebx)
80100178:	74 0e                	je     80100188 <bread+0xb8>
    iderw(b);
  }
  return b;
}
8010017a:	8d 65 f4             	lea    -0xc(%ebp),%esp
8010017d:	89 d8                	mov    %ebx,%eax
8010017f:	5b                   	pop    %ebx
80100180:	5e                   	pop    %esi
80100181:	5f                   	pop    %edi
80100182:	5d                   	pop    %ebp
80100183:	c3                   	ret
80100184:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    iderw(b);
80100188:	83 ec 0c             	sub    $0xc,%esp
8010018b:	53                   	push   %ebx
8010018c:	e8 3f 21 00 00       	call   801022d0 <iderw>
80100191:	83 c4 10             	add    $0x10,%esp
}
80100194:	8d 65 f4             	lea    -0xc(%ebp),%esp
80100197:	89 d8                	mov    %ebx,%eax
80100199:	5b                   	pop    %ebx
8010019a:	5e                   	pop    %esi
8010019b:	5f                   	pop    %edi
8010019c:	5d                   	pop    %ebp
8010019d:	c3                   	ret
  panic("bget: no buffers");
8010019e:	83 ec 0c             	sub    $0xc,%esp
801001a1:	68 6e 71 10 80       	push   $0x8010716e
801001a6:	e8 d5 01 00 00       	call   80100380 <panic>
801001ab:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi

801001b0 <bwrite>:

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
801001b0:	55                   	push   %ebp
801001b1:	89 e5                	mov    %esp,%ebp
801001b3:	53                   	push   %ebx
801001b4:	83 ec 10             	sub    $0x10,%esp
801001b7:	8b 5d 08             	mov    0x8(%ebp),%ebx
  if(!holdingsleep(&b->lock))
801001ba:	8d 43 0c             	lea    0xc(%ebx),%eax
801001bd:	50                   	push   %eax
801001be:	e8 9d 41 00 00       	call   80104360 <holdingsleep>
801001c3:	83 c4 10             	add    $0x10,%esp
801001c6:	85 c0                	test   %eax,%eax
801001c8:	74 0f                	je     801001d9 <bwrite+0x29>
    panic("bwrite");
  b->flags |= B_DIRTY;
801001ca:	83 0b 04             	orl    $0x4,(%ebx)
  iderw(b);
801001cd:	89 5d 08             	mov    %ebx,0x8(%ebp)
}
801001d0:	8b 5d fc             	mov    -0x4(%ebp),%ebx
801001d3:	c9                   	leave
  iderw(b);
801001d4:	e9 f7 20 00 00       	jmp    801022d0 <iderw>
    panic("bwrite");
801001d9:	83 ec 0c             	sub    $0xc,%esp
801001dc:	68 7f 71 10 80       	push   $0x8010717f
801001e1:	e8 9a 01 00 00       	call   80100380 <panic>
801001e6:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801001ed:	00 
801001ee:	66 90                	xchg   %ax,%ax

801001f0 <brelse>:

// Release a locked buffer.
// Move to the head of the MRU list.
void
brelse(struct buf *b)
{
801001f0:	55                   	push   %ebp
801001f1:	89 e5                	mov    %esp,%ebp
801001f3:	56                   	push   %esi
801001f4:	53                   	push   %ebx
801001f5:	8b 5d 08             	mov    0x8(%ebp),%ebx
  if(!holdingsleep(&b->lock))
801001f8:	8d 73 0c             	lea    0xc(%ebx),%esi
801001fb:	83 ec 0c             	sub    $0xc,%esp
801001fe:	56                   	push   %esi
801001ff:	e8 5c 41 00 00       	call   80104360 <holdingsleep>
80100204:	83 c4 10             	add    $0x10,%esp
80100207:	85 c0                	test   %eax,%eax
80100209:	74 63                	je     8010026e <brelse+0x7e>
    panic("brelse");

  releasesleep(&b->lock);
8010020b:	83 ec 0c             	sub    $0xc,%esp
8010020e:	56                   	push   %esi
8010020f:	e8 0c 41 00 00       	call   80104320 <releasesleep>

  acquire(&bcache.lock);
80100214:	c7 04 24 20 a5 10 80 	movl   $0x8010a520,(%esp)
8010021b:	e8 80 43 00 00       	call   801045a0 <acquire>
  b->refcnt--;
80100220:	8b 43 4c             	mov    0x4c(%ebx),%eax
  if (b->refcnt == 0) {
80100223:	83 c4 10             	add    $0x10,%esp
  b->refcnt--;
80100226:	83 e8 01             	sub    $0x1,%eax
80100229:	89 43 4c             	mov    %eax,0x4c(%ebx)
  if (b->refcnt == 0) {
8010022c:	85 c0                	test   %eax,%eax
8010022e:	75 2c                	jne    8010025c <brelse+0x6c>
    // no one is waiting for it.
    b->next->prev = b->prev;
80100230:	8b 53 54             	mov    0x54(%ebx),%edx
80100233:	8b 43 50             	mov    0x50(%ebx),%eax
80100236:	89 42 50             	mov    %eax,0x50(%edx)
    b->prev->next = b->next;
80100239:	8b 53 54             	mov    0x54(%ebx),%edx
8010023c:	89 50 54             	mov    %edx,0x54(%eax)
    b->next = bcache.head.next;
8010023f:	a1 70 ec 10 80       	mov    0x8010ec70,%eax
    b->prev = &bcache.head;
80100244:	c7 43 50 1c ec 10 80 	movl   $0x8010ec1c,0x50(%ebx)
    b->next = bcache.head.next;
8010024b:	89 43 54             	mov    %eax,0x54(%ebx)
    bcache.head.next->prev = b;
8010024e:	a1 70 ec 10 80       	mov    0x8010ec70,%eax
80100253:	89 58 50             	mov    %ebx,0x50(%eax)
    bcache.head.next = b;
80100256:	89 1d 70 ec 10 80    	mov    %ebx,0x8010ec70
  }
  
  release(&bcache.lock);
8010025c:	c7 45 08 20 a5 10 80 	movl   $0x8010a520,0x8(%ebp)
}
80100263:	8d 65 f8             	lea    -0x8(%ebp),%esp
80100266:	5b                   	pop    %ebx
80100267:	5e                   	pop    %esi
80100268:	5d                   	pop    %ebp
  release(&bcache.lock);
80100269:	e9 d2 42 00 00       	jmp    80104540 <release>
    panic("brelse");
8010026e:	83 ec 0c             	sub    $0xc,%esp
80100271:	68 86 71 10 80       	push   $0x80107186
80100276:	e8 05 01 00 00       	call   80100380 <panic>
8010027b:	66 90                	xchg   %ax,%ax
8010027d:	66 90                	xchg   %ax,%ax
8010027f:	90                   	nop

80100280 <consoleread>:
  }
}

int
consoleread(struct inode *ip, char *dst, int n)
{
80100280:	55                   	push   %ebp
80100281:	89 e5                	mov    %esp,%ebp
80100283:	57                   	push   %edi
80100284:	56                   	push   %esi
80100285:	53                   	push   %ebx
80100286:	83 ec 18             	sub    $0x18,%esp
80100289:	8b 5d 10             	mov    0x10(%ebp),%ebx
8010028c:	8b 75 0c             	mov    0xc(%ebp),%esi
  uint target;
  int c;

  iunlock(ip);
8010028f:	ff 75 08             	push   0x8(%ebp)
  target = n;
80100292:	89 df                	mov    %ebx,%edi
  iunlock(ip);
80100294:	e8 e7 15 00 00       	call   80101880 <iunlock>
  acquire(&cons.lock);
80100299:	c7 04 24 20 ef 10 80 	movl   $0x8010ef20,(%esp)
801002a0:	e8 fb 42 00 00       	call   801045a0 <acquire>
  while(n > 0){
801002a5:	83 c4 10             	add    $0x10,%esp
801002a8:	85 db                	test   %ebx,%ebx
801002aa:	0f 8e 94 00 00 00    	jle    80100344 <consoleread+0xc4>
    while(input.r == input.w){
801002b0:	a1 00 ef 10 80       	mov    0x8010ef00,%eax
801002b5:	39 05 04 ef 10 80    	cmp    %eax,0x8010ef04
801002bb:	74 25                	je     801002e2 <consoleread+0x62>
801002bd:	eb 59                	jmp    80100318 <consoleread+0x98>
801002bf:	90                   	nop
      if(myproc()->killed){
        release(&cons.lock);
        ilock(ip);
        return -1;
      }
      sleep(&input.r, &cons.lock);
801002c0:	83 ec 08             	sub    $0x8,%esp
801002c3:	68 20 ef 10 80       	push   $0x8010ef20
801002c8:	68 00 ef 10 80       	push   $0x8010ef00
801002cd:	e8 4e 3d 00 00       	call   80104020 <sleep>
    while(input.r == input.w){
801002d2:	a1 00 ef 10 80       	mov    0x8010ef00,%eax
801002d7:	83 c4 10             	add    $0x10,%esp
801002da:	3b 05 04 ef 10 80    	cmp    0x8010ef04,%eax
801002e0:	75 36                	jne    80100318 <consoleread+0x98>
      if(myproc()->killed){
801002e2:	e8 79 36 00 00       	call   80103960 <myproc>
801002e7:	8b 48 24             	mov    0x24(%eax),%ecx
801002ea:	85 c9                	test   %ecx,%ecx
801002ec:	74 d2                	je     801002c0 <consoleread+0x40>
        release(&cons.lock);
801002ee:	83 ec 0c             	sub    $0xc,%esp
801002f1:	68 20 ef 10 80       	push   $0x8010ef20
801002f6:	e8 45 42 00 00       	call   80104540 <release>
        ilock(ip);
801002fb:	5a                   	pop    %edx
801002fc:	ff 75 08             	push   0x8(%ebp)
801002ff:	e8 9c 14 00 00       	call   801017a0 <ilock>
        return -1;
80100304:	83 c4 10             	add    $0x10,%esp
  }
  release(&cons.lock);
  ilock(ip);

  return target - n;
}
80100307:	8d 65 f4             	lea    -0xc(%ebp),%esp
        return -1;
8010030a:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
}
8010030f:	5b                   	pop    %ebx
80100310:	5e                   	pop    %esi
80100311:	5f                   	pop    %edi
80100312:	5d                   	pop    %ebp
80100313:	c3                   	ret
80100314:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    c = input.buf[input.r++ % INPUT_BUF];
80100318:	8d 50 01             	lea    0x1(%eax),%edx
8010031b:	89 15 00 ef 10 80    	mov    %edx,0x8010ef00
80100321:	89 c2                	mov    %eax,%edx
80100323:	83 e2 7f             	and    $0x7f,%edx
80100326:	0f be 8a 80 ee 10 80 	movsbl -0x7fef1180(%edx),%ecx
    if(c == C('D')){  // EOF
8010032d:	80 f9 04             	cmp    $0x4,%cl
80100330:	74 37                	je     80100369 <consoleread+0xe9>
    *dst++ = c;
80100332:	83 c6 01             	add    $0x1,%esi
    --n;
80100335:	83 eb 01             	sub    $0x1,%ebx
    *dst++ = c;
80100338:	88 4e ff             	mov    %cl,-0x1(%esi)
    if(c == '\n')
8010033b:	83 f9 0a             	cmp    $0xa,%ecx
8010033e:	0f 85 64 ff ff ff    	jne    801002a8 <consoleread+0x28>
  release(&cons.lock);
80100344:	83 ec 0c             	sub    $0xc,%esp
80100347:	68 20 ef 10 80       	push   $0x8010ef20
8010034c:	e8 ef 41 00 00       	call   80104540 <release>
  ilock(ip);
80100351:	58                   	pop    %eax
80100352:	ff 75 08             	push   0x8(%ebp)
80100355:	e8 46 14 00 00       	call   801017a0 <ilock>
  return target - n;
8010035a:	89 f8                	mov    %edi,%eax
8010035c:	83 c4 10             	add    $0x10,%esp
}
8010035f:	8d 65 f4             	lea    -0xc(%ebp),%esp
  return target - n;
80100362:	29 d8                	sub    %ebx,%eax
}
80100364:	5b                   	pop    %ebx
80100365:	5e                   	pop    %esi
80100366:	5f                   	pop    %edi
80100367:	5d                   	pop    %ebp
80100368:	c3                   	ret
      if(n < target){
80100369:	39 fb                	cmp    %edi,%ebx
8010036b:	73 d7                	jae    80100344 <consoleread+0xc4>
        input.r--;
8010036d:	a3 00 ef 10 80       	mov    %eax,0x8010ef00
80100372:	eb d0                	jmp    80100344 <consoleread+0xc4>
80100374:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010037b:	00 
8010037c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi

80100380 <panic>:
{
80100380:	55                   	push   %ebp
80100381:	89 e5                	mov    %esp,%ebp
80100383:	56                   	push   %esi
80100384:	53                   	push   %ebx
80100385:	83 ec 30             	sub    $0x30,%esp
}

static inline void
cli(void)
{
  asm volatile("cli");
80100388:	fa                   	cli
  cons.locking = 0;
80100389:	c7 05 54 ef 10 80 00 	movl   $0x0,0x8010ef54
80100390:	00 00 00 
  getcallerpcs(&s, pcs);
80100393:	8d 5d d0             	lea    -0x30(%ebp),%ebx
80100396:	8d 75 f8             	lea    -0x8(%ebp),%esi
  cprintf("lapicid %d: panic: ", lapicid());
80100399:	e8 32 25 00 00       	call   801028d0 <lapicid>
8010039e:	83 ec 08             	sub    $0x8,%esp
801003a1:	50                   	push   %eax
801003a2:	68 8d 71 10 80       	push   $0x8010718d
801003a7:	e8 04 03 00 00       	call   801006b0 <cprintf>
  cprintf(s);
801003ac:	58                   	pop    %eax
801003ad:	ff 75 08             	push   0x8(%ebp)
801003b0:	e8 fb 02 00 00       	call   801006b0 <cprintf>
  cprintf("\n");
801003b5:	c7 04 24 0f 76 10 80 	movl   $0x8010760f,(%esp)
801003bc:	e8 ef 02 00 00       	call   801006b0 <cprintf>
  getcallerpcs(&s, pcs);
801003c1:	8d 45 08             	lea    0x8(%ebp),%eax
801003c4:	5a                   	pop    %edx
801003c5:	59                   	pop    %ecx
801003c6:	53                   	push   %ebx
801003c7:	50                   	push   %eax
801003c8:	e8 03 40 00 00       	call   801043d0 <getcallerpcs>
  for(i=0; i<10; i++)
801003cd:	83 c4 10             	add    $0x10,%esp
    cprintf(" %p", pcs[i]);
801003d0:	83 ec 08             	sub    $0x8,%esp
801003d3:	ff 33                	push   (%ebx)
  for(i=0; i<10; i++)
801003d5:	83 c3 04             	add    $0x4,%ebx
    cprintf(" %p", pcs[i]);
801003d8:	68 a1 71 10 80       	push   $0x801071a1
801003dd:	e8 ce 02 00 00       	call   801006b0 <cprintf>
  for(i=0; i<10; i++)
801003e2:	83 c4 10             	add    $0x10,%esp
801003e5:	39 f3                	cmp    %esi,%ebx
801003e7:	75 e7                	jne    801003d0 <panic+0x50>
  panicked = 1; // freeze other CPU
801003e9:	c7 05 58 ef 10 80 01 	movl   $0x1,0x8010ef58
801003f0:	00 00 00 
  for(;;)
801003f3:	eb fe                	jmp    801003f3 <panic+0x73>
801003f5:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801003fc:	00 
801003fd:	8d 76 00             	lea    0x0(%esi),%esi

80100400 <consputc.part.0>:
consputc(int c)
80100400:	55                   	push   %ebp
80100401:	89 e5                	mov    %esp,%ebp
80100403:	57                   	push   %edi
80100404:	56                   	push   %esi
80100405:	53                   	push   %ebx
80100406:	83 ec 1c             	sub    $0x1c,%esp
  if(c == BACKSPACE){
80100409:	3d 00 01 00 00       	cmp    $0x100,%eax
8010040e:	0f 84 cc 00 00 00    	je     801004e0 <consputc.part.0+0xe0>
    uartputc(c);
80100414:	83 ec 0c             	sub    $0xc,%esp
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
80100417:	bf d4 03 00 00       	mov    $0x3d4,%edi
8010041c:	89 c3                	mov    %eax,%ebx
8010041e:	50                   	push   %eax
8010041f:	e8 8c 58 00 00       	call   80105cb0 <uartputc>
80100424:	b8 0e 00 00 00       	mov    $0xe,%eax
80100429:	89 fa                	mov    %edi,%edx
8010042b:	ee                   	out    %al,(%dx)
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
8010042c:	be d5 03 00 00       	mov    $0x3d5,%esi
80100431:	89 f2                	mov    %esi,%edx
80100433:	ec                   	in     (%dx),%al
  pos = inb(CRTPORT+1) << 8;
80100434:	0f b6 c8             	movzbl %al,%ecx
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
80100437:	89 fa                	mov    %edi,%edx
80100439:	b8 0f 00 00 00       	mov    $0xf,%eax
8010043e:	c1 e1 08             	shl    $0x8,%ecx
80100441:	ee                   	out    %al,(%dx)
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
80100442:	89 f2                	mov    %esi,%edx
80100444:	ec                   	in     (%dx),%al
  pos |= inb(CRTPORT+1);
80100445:	0f b6 c0             	movzbl %al,%eax
  if(c == '\n')
80100448:	83 c4 10             	add    $0x10,%esp
  pos |= inb(CRTPORT+1);
8010044b:	09 c8                	or     %ecx,%eax
  if(c == '\n')
8010044d:	83 fb 0a             	cmp    $0xa,%ebx
80100450:	75 76                	jne    801004c8 <consputc.part.0+0xc8>
    pos += 80 - pos%80;
80100452:	ba cd cc cc cc       	mov    $0xcccccccd,%edx
80100457:	f7 e2                	mul    %edx
80100459:	c1 ea 06             	shr    $0x6,%edx
8010045c:	8d 04 92             	lea    (%edx,%edx,4),%eax
8010045f:	c1 e0 04             	shl    $0x4,%eax
80100462:	8d 70 50             	lea    0x50(%eax),%esi
  if(pos < 0 || pos > 25*80)
80100465:	81 fe d0 07 00 00    	cmp    $0x7d0,%esi
8010046b:	0f 8f 2f 01 00 00    	jg     801005a0 <consputc.part.0+0x1a0>
  if((pos/80) >= 24){  // Scroll up.
80100471:	81 fe 7f 07 00 00    	cmp    $0x77f,%esi
80100477:	0f 8f c3 00 00 00    	jg     80100540 <consputc.part.0+0x140>
  outb(CRTPORT+1, pos>>8);
8010047d:	89 f0                	mov    %esi,%eax
  crt[pos] = ' ' | 0x0700;
8010047f:	8d b4 36 00 80 0b 80 	lea    -0x7ff48000(%esi,%esi,1),%esi
  outb(CRTPORT+1, pos);
80100486:	88 45 e7             	mov    %al,-0x19(%ebp)
  outb(CRTPORT+1, pos>>8);
80100489:	0f b6 fc             	movzbl %ah,%edi
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
8010048c:	bb d4 03 00 00       	mov    $0x3d4,%ebx
80100491:	b8 0e 00 00 00       	mov    $0xe,%eax
80100496:	89 da                	mov    %ebx,%edx
80100498:	ee                   	out    %al,(%dx)
80100499:	b9 d5 03 00 00       	mov    $0x3d5,%ecx
8010049e:	89 f8                	mov    %edi,%eax
801004a0:	89 ca                	mov    %ecx,%edx
801004a2:	ee                   	out    %al,(%dx)
801004a3:	b8 0f 00 00 00       	mov    $0xf,%eax
801004a8:	89 da                	mov    %ebx,%edx
801004aa:	ee                   	out    %al,(%dx)
801004ab:	0f b6 45 e7          	movzbl -0x19(%ebp),%eax
801004af:	89 ca                	mov    %ecx,%edx
801004b1:	ee                   	out    %al,(%dx)
  crt[pos] = ' ' | 0x0700;
801004b2:	b8 20 07 00 00       	mov    $0x720,%eax
801004b7:	66 89 06             	mov    %ax,(%esi)
}
801004ba:	8d 65 f4             	lea    -0xc(%ebp),%esp
801004bd:	5b                   	pop    %ebx
801004be:	5e                   	pop    %esi
801004bf:	5f                   	pop    %edi
801004c0:	5d                   	pop    %ebp
801004c1:	c3                   	ret
801004c2:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    crt[pos++] = (c&0xff) | 0x0700;  // black on white
801004c8:	0f b6 db             	movzbl %bl,%ebx
801004cb:	8d 70 01             	lea    0x1(%eax),%esi
801004ce:	80 cf 07             	or     $0x7,%bh
801004d1:	66 89 9c 00 00 80 0b 	mov    %bx,-0x7ff48000(%eax,%eax,1)
801004d8:	80 
801004d9:	eb 8a                	jmp    80100465 <consputc.part.0+0x65>
801004db:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
    uartputc('\b'); uartputc(' '); uartputc('\b');
801004e0:	83 ec 0c             	sub    $0xc,%esp
801004e3:	be d4 03 00 00       	mov    $0x3d4,%esi
801004e8:	6a 08                	push   $0x8
801004ea:	e8 c1 57 00 00       	call   80105cb0 <uartputc>
801004ef:	c7 04 24 20 00 00 00 	movl   $0x20,(%esp)
801004f6:	e8 b5 57 00 00       	call   80105cb0 <uartputc>
801004fb:	c7 04 24 08 00 00 00 	movl   $0x8,(%esp)
80100502:	e8 a9 57 00 00       	call   80105cb0 <uartputc>
80100507:	b8 0e 00 00 00       	mov    $0xe,%eax
8010050c:	89 f2                	mov    %esi,%edx
8010050e:	ee                   	out    %al,(%dx)
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
8010050f:	bb d5 03 00 00       	mov    $0x3d5,%ebx
80100514:	89 da                	mov    %ebx,%edx
80100516:	ec                   	in     (%dx),%al
  pos = inb(CRTPORT+1) << 8;
80100517:	0f b6 c8             	movzbl %al,%ecx
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
8010051a:	89 f2                	mov    %esi,%edx
8010051c:	b8 0f 00 00 00       	mov    $0xf,%eax
80100521:	c1 e1 08             	shl    $0x8,%ecx
80100524:	ee                   	out    %al,(%dx)
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
80100525:	89 da                	mov    %ebx,%edx
80100527:	ec                   	in     (%dx),%al
  pos |= inb(CRTPORT+1);
80100528:	0f b6 f0             	movzbl %al,%esi
    if(pos > 0) --pos;
8010052b:	83 c4 10             	add    $0x10,%esp
8010052e:	09 ce                	or     %ecx,%esi
80100530:	74 5e                	je     80100590 <consputc.part.0+0x190>
80100532:	83 ee 01             	sub    $0x1,%esi
80100535:	e9 2b ff ff ff       	jmp    80100465 <consputc.part.0+0x65>
8010053a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    memmove(crt, crt+80, sizeof(crt[0])*23*80);
80100540:	83 ec 04             	sub    $0x4,%esp
    pos -= 80;
80100543:	8d 5e b0             	lea    -0x50(%esi),%ebx
    memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
80100546:	8d b4 36 60 7f 0b 80 	lea    -0x7ff480a0(%esi,%esi,1),%esi
  outb(CRTPORT+1, pos);
8010054d:	bf 07 00 00 00       	mov    $0x7,%edi
    memmove(crt, crt+80, sizeof(crt[0])*23*80);
80100552:	68 60 0e 00 00       	push   $0xe60
80100557:	68 a0 80 0b 80       	push   $0x800b80a0
8010055c:	68 00 80 0b 80       	push   $0x800b8000
80100561:	e8 ca 41 00 00       	call   80104730 <memmove>
    memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
80100566:	b8 80 07 00 00       	mov    $0x780,%eax
8010056b:	83 c4 0c             	add    $0xc,%esp
8010056e:	29 d8                	sub    %ebx,%eax
80100570:	01 c0                	add    %eax,%eax
80100572:	50                   	push   %eax
80100573:	6a 00                	push   $0x0
80100575:	56                   	push   %esi
80100576:	e8 25 41 00 00       	call   801046a0 <memset>
  outb(CRTPORT+1, pos);
8010057b:	88 5d e7             	mov    %bl,-0x19(%ebp)
8010057e:	83 c4 10             	add    $0x10,%esp
80100581:	e9 06 ff ff ff       	jmp    8010048c <consputc.part.0+0x8c>
80100586:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010058d:	00 
8010058e:	66 90                	xchg   %ax,%ax
80100590:	c6 45 e7 00          	movb   $0x0,-0x19(%ebp)
80100594:	be 00 80 0b 80       	mov    $0x800b8000,%esi
80100599:	31 ff                	xor    %edi,%edi
8010059b:	e9 ec fe ff ff       	jmp    8010048c <consputc.part.0+0x8c>
    panic("pos under/overflow");
801005a0:	83 ec 0c             	sub    $0xc,%esp
801005a3:	68 a5 71 10 80       	push   $0x801071a5
801005a8:	e8 d3 fd ff ff       	call   80100380 <panic>
801005ad:	8d 76 00             	lea    0x0(%esi),%esi

801005b0 <consolewrite>:

int
consolewrite(struct inode *ip, char *buf, int n)
{
801005b0:	55                   	push   %ebp
801005b1:	89 e5                	mov    %esp,%ebp
801005b3:	57                   	push   %edi
801005b4:	56                   	push   %esi
801005b5:	53                   	push   %ebx
801005b6:	83 ec 18             	sub    $0x18,%esp
801005b9:	8b 75 10             	mov    0x10(%ebp),%esi
  int i;

  iunlock(ip);
801005bc:	ff 75 08             	push   0x8(%ebp)
801005bf:	e8 bc 12 00 00       	call   80101880 <iunlock>
  acquire(&cons.lock);
801005c4:	c7 04 24 20 ef 10 80 	movl   $0x8010ef20,(%esp)
801005cb:	e8 d0 3f 00 00       	call   801045a0 <acquire>
  for(i = 0; i < n; i++)
801005d0:	83 c4 10             	add    $0x10,%esp
801005d3:	85 f6                	test   %esi,%esi
801005d5:	7e 25                	jle    801005fc <consolewrite+0x4c>
801005d7:	8b 5d 0c             	mov    0xc(%ebp),%ebx
801005da:	8d 3c 33             	lea    (%ebx,%esi,1),%edi
  if(panicked){
801005dd:	8b 15 58 ef 10 80    	mov    0x8010ef58,%edx
    consputc(buf[i] & 0xff);
801005e3:	0f b6 03             	movzbl (%ebx),%eax
  if(panicked){
801005e6:	85 d2                	test   %edx,%edx
801005e8:	74 06                	je     801005f0 <consolewrite+0x40>
  asm volatile("cli");
801005ea:	fa                   	cli
    for(;;)
801005eb:	eb fe                	jmp    801005eb <consolewrite+0x3b>
801005ed:	8d 76 00             	lea    0x0(%esi),%esi
801005f0:	e8 0b fe ff ff       	call   80100400 <consputc.part.0>
  for(i = 0; i < n; i++)
801005f5:	83 c3 01             	add    $0x1,%ebx
801005f8:	39 fb                	cmp    %edi,%ebx
801005fa:	75 e1                	jne    801005dd <consolewrite+0x2d>
  release(&cons.lock);
801005fc:	83 ec 0c             	sub    $0xc,%esp
801005ff:	68 20 ef 10 80       	push   $0x8010ef20
80100604:	e8 37 3f 00 00       	call   80104540 <release>
  ilock(ip);
80100609:	58                   	pop    %eax
8010060a:	ff 75 08             	push   0x8(%ebp)
8010060d:	e8 8e 11 00 00       	call   801017a0 <ilock>

  return n;
}
80100612:	8d 65 f4             	lea    -0xc(%ebp),%esp
80100615:	89 f0                	mov    %esi,%eax
80100617:	5b                   	pop    %ebx
80100618:	5e                   	pop    %esi
80100619:	5f                   	pop    %edi
8010061a:	5d                   	pop    %ebp
8010061b:	c3                   	ret
8010061c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi

80100620 <printint>:
{
80100620:	55                   	push   %ebp
80100621:	89 e5                	mov    %esp,%ebp
80100623:	57                   	push   %edi
80100624:	56                   	push   %esi
80100625:	53                   	push   %ebx
80100626:	89 d3                	mov    %edx,%ebx
80100628:	83 ec 2c             	sub    $0x2c,%esp
  if(sign && (sign = xx < 0))
8010062b:	85 c0                	test   %eax,%eax
8010062d:	79 05                	jns    80100634 <printint+0x14>
8010062f:	83 e1 01             	and    $0x1,%ecx
80100632:	75 64                	jne    80100698 <printint+0x78>
    x = xx;
80100634:	c7 45 d4 00 00 00 00 	movl   $0x0,-0x2c(%ebp)
8010063b:	89 c1                	mov    %eax,%ecx
  i = 0;
8010063d:	31 f6                	xor    %esi,%esi
8010063f:	90                   	nop
    buf[i++] = digits[x % base];
80100640:	89 c8                	mov    %ecx,%eax
80100642:	31 d2                	xor    %edx,%edx
80100644:	89 f7                	mov    %esi,%edi
80100646:	f7 f3                	div    %ebx
80100648:	8d 76 01             	lea    0x1(%esi),%esi
8010064b:	0f b6 92 60 76 10 80 	movzbl -0x7fef89a0(%edx),%edx
80100652:	88 54 35 d7          	mov    %dl,-0x29(%ebp,%esi,1)
  }while((x /= base) != 0);
80100656:	89 ca                	mov    %ecx,%edx
80100658:	89 c1                	mov    %eax,%ecx
8010065a:	39 da                	cmp    %ebx,%edx
8010065c:	73 e2                	jae    80100640 <printint+0x20>
  if(sign)
8010065e:	8b 4d d4             	mov    -0x2c(%ebp),%ecx
80100661:	85 c9                	test   %ecx,%ecx
80100663:	74 07                	je     8010066c <printint+0x4c>
    buf[i++] = '-';
80100665:	c6 44 35 d8 2d       	movb   $0x2d,-0x28(%ebp,%esi,1)
  while(--i >= 0)
8010066a:	89 f7                	mov    %esi,%edi
8010066c:	8d 5d d8             	lea    -0x28(%ebp),%ebx
8010066f:	01 df                	add    %ebx,%edi
  if(panicked){
80100671:	8b 15 58 ef 10 80    	mov    0x8010ef58,%edx
    consputc(buf[i]);
80100677:	0f be 07             	movsbl (%edi),%eax
  if(panicked){
8010067a:	85 d2                	test   %edx,%edx
8010067c:	74 0a                	je     80100688 <printint+0x68>
8010067e:	fa                   	cli
    for(;;)
8010067f:	eb fe                	jmp    8010067f <printint+0x5f>
80100681:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
80100688:	e8 73 fd ff ff       	call   80100400 <consputc.part.0>
  while(--i >= 0)
8010068d:	8d 47 ff             	lea    -0x1(%edi),%eax
80100690:	39 df                	cmp    %ebx,%edi
80100692:	74 11                	je     801006a5 <printint+0x85>
80100694:	89 c7                	mov    %eax,%edi
80100696:	eb d9                	jmp    80100671 <printint+0x51>
    x = -xx;
80100698:	f7 d8                	neg    %eax
  if(sign && (sign = xx < 0))
8010069a:	c7 45 d4 01 00 00 00 	movl   $0x1,-0x2c(%ebp)
    x = -xx;
801006a1:	89 c1                	mov    %eax,%ecx
801006a3:	eb 98                	jmp    8010063d <printint+0x1d>
}
801006a5:	83 c4 2c             	add    $0x2c,%esp
801006a8:	5b                   	pop    %ebx
801006a9:	5e                   	pop    %esi
801006aa:	5f                   	pop    %edi
801006ab:	5d                   	pop    %ebp
801006ac:	c3                   	ret
801006ad:	8d 76 00             	lea    0x0(%esi),%esi

801006b0 <cprintf>:
{
801006b0:	55                   	push   %ebp
801006b1:	89 e5                	mov    %esp,%ebp
801006b3:	57                   	push   %edi
801006b4:	56                   	push   %esi
801006b5:	53                   	push   %ebx
801006b6:	83 ec 1c             	sub    $0x1c,%esp
  locking = cons.locking;
801006b9:	8b 3d 54 ef 10 80    	mov    0x8010ef54,%edi
  if (fmt == 0)
801006bf:	8b 75 08             	mov    0x8(%ebp),%esi
  if(locking)
801006c2:	85 ff                	test   %edi,%edi
801006c4:	0f 85 06 01 00 00    	jne    801007d0 <cprintf+0x120>
  if (fmt == 0)
801006ca:	85 f6                	test   %esi,%esi
801006cc:	0f 84 b7 01 00 00    	je     80100889 <cprintf+0x1d9>
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
801006d2:	0f b6 06             	movzbl (%esi),%eax
801006d5:	85 c0                	test   %eax,%eax
801006d7:	74 5f                	je     80100738 <cprintf+0x88>
  argp = (uint*)(void*)(&fmt + 1);
801006d9:	8d 55 0c             	lea    0xc(%ebp),%edx
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
801006dc:	89 7d e4             	mov    %edi,-0x1c(%ebp)
801006df:	31 db                	xor    %ebx,%ebx
801006e1:	89 d7                	mov    %edx,%edi
    if(c != '%'){
801006e3:	83 f8 25             	cmp    $0x25,%eax
801006e6:	75 58                	jne    80100740 <cprintf+0x90>
    c = fmt[++i] & 0xff;
801006e8:	83 c3 01             	add    $0x1,%ebx
801006eb:	0f b6 0c 1e          	movzbl (%esi,%ebx,1),%ecx
    if(c == 0)
801006ef:	85 c9                	test   %ecx,%ecx
801006f1:	74 3a                	je     8010072d <cprintf+0x7d>
    switch(c){
801006f3:	83 f9 70             	cmp    $0x70,%ecx
801006f6:	0f 84 b4 00 00 00    	je     801007b0 <cprintf+0x100>
801006fc:	7f 72                	jg     80100770 <cprintf+0xc0>
801006fe:	83 f9 25             	cmp    $0x25,%ecx
80100701:	74 4d                	je     80100750 <cprintf+0xa0>
80100703:	83 f9 64             	cmp    $0x64,%ecx
80100706:	75 76                	jne    8010077e <cprintf+0xce>
      printint(*argp++, 10, 1);
80100708:	8d 47 04             	lea    0x4(%edi),%eax
8010070b:	b9 01 00 00 00       	mov    $0x1,%ecx
80100710:	ba 0a 00 00 00       	mov    $0xa,%edx
80100715:	89 45 e0             	mov    %eax,-0x20(%ebp)
80100718:	8b 07                	mov    (%edi),%eax
8010071a:	e8 01 ff ff ff       	call   80100620 <printint>
8010071f:	8b 7d e0             	mov    -0x20(%ebp),%edi
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
80100722:	83 c3 01             	add    $0x1,%ebx
80100725:	0f b6 04 1e          	movzbl (%esi,%ebx,1),%eax
80100729:	85 c0                	test   %eax,%eax
8010072b:	75 b6                	jne    801006e3 <cprintf+0x33>
8010072d:	8b 7d e4             	mov    -0x1c(%ebp),%edi
  if(locking)
80100730:	85 ff                	test   %edi,%edi
80100732:	0f 85 bb 00 00 00    	jne    801007f3 <cprintf+0x143>
}
80100738:	8d 65 f4             	lea    -0xc(%ebp),%esp
8010073b:	5b                   	pop    %ebx
8010073c:	5e                   	pop    %esi
8010073d:	5f                   	pop    %edi
8010073e:	5d                   	pop    %ebp
8010073f:	c3                   	ret
  if(panicked){
80100740:	8b 0d 58 ef 10 80    	mov    0x8010ef58,%ecx
80100746:	85 c9                	test   %ecx,%ecx
80100748:	74 19                	je     80100763 <cprintf+0xb3>
8010074a:	fa                   	cli
    for(;;)
8010074b:	eb fe                	jmp    8010074b <cprintf+0x9b>
8010074d:	8d 76 00             	lea    0x0(%esi),%esi
  if(panicked){
80100750:	8b 0d 58 ef 10 80    	mov    0x8010ef58,%ecx
80100756:	85 c9                	test   %ecx,%ecx
80100758:	0f 85 f2 00 00 00    	jne    80100850 <cprintf+0x1a0>
8010075e:	b8 25 00 00 00       	mov    $0x25,%eax
80100763:	e8 98 fc ff ff       	call   80100400 <consputc.part.0>
      break;
80100768:	eb b8                	jmp    80100722 <cprintf+0x72>
8010076a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    switch(c){
80100770:	83 f9 73             	cmp    $0x73,%ecx
80100773:	0f 84 8f 00 00 00    	je     80100808 <cprintf+0x158>
80100779:	83 f9 78             	cmp    $0x78,%ecx
8010077c:	74 32                	je     801007b0 <cprintf+0x100>
  if(panicked){
8010077e:	8b 15 58 ef 10 80    	mov    0x8010ef58,%edx
80100784:	85 d2                	test   %edx,%edx
80100786:	0f 85 b8 00 00 00    	jne    80100844 <cprintf+0x194>
8010078c:	b8 25 00 00 00       	mov    $0x25,%eax
80100791:	89 4d e0             	mov    %ecx,-0x20(%ebp)
80100794:	e8 67 fc ff ff       	call   80100400 <consputc.part.0>
80100799:	a1 58 ef 10 80       	mov    0x8010ef58,%eax
8010079e:	8b 4d e0             	mov    -0x20(%ebp),%ecx
801007a1:	85 c0                	test   %eax,%eax
801007a3:	0f 84 cd 00 00 00    	je     80100876 <cprintf+0x1c6>
801007a9:	fa                   	cli
    for(;;)
801007aa:	eb fe                	jmp    801007aa <cprintf+0xfa>
801007ac:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
      printint(*argp++, 16, 0);
801007b0:	8d 47 04             	lea    0x4(%edi),%eax
801007b3:	31 c9                	xor    %ecx,%ecx
801007b5:	ba 10 00 00 00       	mov    $0x10,%edx
801007ba:	89 45 e0             	mov    %eax,-0x20(%ebp)
801007bd:	8b 07                	mov    (%edi),%eax
801007bf:	e8 5c fe ff ff       	call   80100620 <printint>
801007c4:	8b 7d e0             	mov    -0x20(%ebp),%edi
      break;
801007c7:	e9 56 ff ff ff       	jmp    80100722 <cprintf+0x72>
801007cc:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    acquire(&cons.lock);
801007d0:	83 ec 0c             	sub    $0xc,%esp
801007d3:	68 20 ef 10 80       	push   $0x8010ef20
801007d8:	e8 c3 3d 00 00       	call   801045a0 <acquire>
  if (fmt == 0)
801007dd:	83 c4 10             	add    $0x10,%esp
801007e0:	85 f6                	test   %esi,%esi
801007e2:	0f 84 a1 00 00 00    	je     80100889 <cprintf+0x1d9>
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
801007e8:	0f b6 06             	movzbl (%esi),%eax
801007eb:	85 c0                	test   %eax,%eax
801007ed:	0f 85 e6 fe ff ff    	jne    801006d9 <cprintf+0x29>
    release(&cons.lock);
801007f3:	83 ec 0c             	sub    $0xc,%esp
801007f6:	68 20 ef 10 80       	push   $0x8010ef20
801007fb:	e8 40 3d 00 00       	call   80104540 <release>
80100800:	83 c4 10             	add    $0x10,%esp
80100803:	e9 30 ff ff ff       	jmp    80100738 <cprintf+0x88>
      if((s = (char*)*argp++) == 0)
80100808:	8b 17                	mov    (%edi),%edx
8010080a:	8d 47 04             	lea    0x4(%edi),%eax
8010080d:	85 d2                	test   %edx,%edx
8010080f:	74 27                	je     80100838 <cprintf+0x188>
      for(; *s; s++)
80100811:	0f b6 0a             	movzbl (%edx),%ecx
      if((s = (char*)*argp++) == 0)
80100814:	89 d7                	mov    %edx,%edi
      for(; *s; s++)
80100816:	84 c9                	test   %cl,%cl
80100818:	74 68                	je     80100882 <cprintf+0x1d2>
8010081a:	89 5d e0             	mov    %ebx,-0x20(%ebp)
8010081d:	89 fb                	mov    %edi,%ebx
8010081f:	89 f7                	mov    %esi,%edi
80100821:	89 c6                	mov    %eax,%esi
80100823:	0f be c1             	movsbl %cl,%eax
  if(panicked){
80100826:	8b 15 58 ef 10 80    	mov    0x8010ef58,%edx
8010082c:	85 d2                	test   %edx,%edx
8010082e:	74 28                	je     80100858 <cprintf+0x1a8>
80100830:	fa                   	cli
    for(;;)
80100831:	eb fe                	jmp    80100831 <cprintf+0x181>
80100833:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
80100838:	b9 28 00 00 00       	mov    $0x28,%ecx
        s = "(null)";
8010083d:	bf b8 71 10 80       	mov    $0x801071b8,%edi
80100842:	eb d6                	jmp    8010081a <cprintf+0x16a>
80100844:	fa                   	cli
    for(;;)
80100845:	eb fe                	jmp    80100845 <cprintf+0x195>
80100847:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010084e:	00 
8010084f:	90                   	nop
80100850:	fa                   	cli
80100851:	eb fe                	jmp    80100851 <cprintf+0x1a1>
80100853:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
80100858:	e8 a3 fb ff ff       	call   80100400 <consputc.part.0>
      for(; *s; s++)
8010085d:	0f be 43 01          	movsbl 0x1(%ebx),%eax
80100861:	83 c3 01             	add    $0x1,%ebx
80100864:	84 c0                	test   %al,%al
80100866:	75 be                	jne    80100826 <cprintf+0x176>
      if((s = (char*)*argp++) == 0)
80100868:	89 f0                	mov    %esi,%eax
8010086a:	8b 5d e0             	mov    -0x20(%ebp),%ebx
8010086d:	89 fe                	mov    %edi,%esi
8010086f:	89 c7                	mov    %eax,%edi
80100871:	e9 ac fe ff ff       	jmp    80100722 <cprintf+0x72>
80100876:	89 c8                	mov    %ecx,%eax
80100878:	e8 83 fb ff ff       	call   80100400 <consputc.part.0>
      break;
8010087d:	e9 a0 fe ff ff       	jmp    80100722 <cprintf+0x72>
      if((s = (char*)*argp++) == 0)
80100882:	89 c7                	mov    %eax,%edi
80100884:	e9 99 fe ff ff       	jmp    80100722 <cprintf+0x72>
    panic("null fmt");
80100889:	83 ec 0c             	sub    $0xc,%esp
8010088c:	68 bf 71 10 80       	push   $0x801071bf
80100891:	e8 ea fa ff ff       	call   80100380 <panic>
80100896:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010089d:	00 
8010089e:	66 90                	xchg   %ax,%ax

801008a0 <consoleintr>:
{
801008a0:	55                   	push   %ebp
801008a1:	89 e5                	mov    %esp,%ebp
801008a3:	57                   	push   %edi
  int c, doprocdump = 0;
801008a4:	31 ff                	xor    %edi,%edi
{
801008a6:	56                   	push   %esi
801008a7:	53                   	push   %ebx
801008a8:	83 ec 18             	sub    $0x18,%esp
801008ab:	8b 75 08             	mov    0x8(%ebp),%esi
  acquire(&cons.lock);
801008ae:	68 20 ef 10 80       	push   $0x8010ef20
801008b3:	e8 e8 3c 00 00       	call   801045a0 <acquire>
  while((c = getc()) >= 0){
801008b8:	83 c4 10             	add    $0x10,%esp
801008bb:	ff d6                	call   *%esi
801008bd:	89 c3                	mov    %eax,%ebx
801008bf:	85 c0                	test   %eax,%eax
801008c1:	78 22                	js     801008e5 <consoleintr+0x45>
    switch(c){
801008c3:	83 fb 15             	cmp    $0x15,%ebx
801008c6:	74 47                	je     8010090f <consoleintr+0x6f>
801008c8:	7f 76                	jg     80100940 <consoleintr+0xa0>
801008ca:	83 fb 08             	cmp    $0x8,%ebx
801008cd:	74 76                	je     80100945 <consoleintr+0xa5>
801008cf:	83 fb 10             	cmp    $0x10,%ebx
801008d2:	0f 85 f8 00 00 00    	jne    801009d0 <consoleintr+0x130>
  while((c = getc()) >= 0){
801008d8:	ff d6                	call   *%esi
    switch(c){
801008da:	bf 01 00 00 00       	mov    $0x1,%edi
  while((c = getc()) >= 0){
801008df:	89 c3                	mov    %eax,%ebx
801008e1:	85 c0                	test   %eax,%eax
801008e3:	79 de                	jns    801008c3 <consoleintr+0x23>
  release(&cons.lock);
801008e5:	83 ec 0c             	sub    $0xc,%esp
801008e8:	68 20 ef 10 80       	push   $0x8010ef20
801008ed:	e8 4e 3c 00 00       	call   80104540 <release>
  if(doprocdump) {
801008f2:	83 c4 10             	add    $0x10,%esp
801008f5:	85 ff                	test   %edi,%edi
801008f7:	0f 85 4b 01 00 00    	jne    80100a48 <consoleintr+0x1a8>
}
801008fd:	8d 65 f4             	lea    -0xc(%ebp),%esp
80100900:	5b                   	pop    %ebx
80100901:	5e                   	pop    %esi
80100902:	5f                   	pop    %edi
80100903:	5d                   	pop    %ebp
80100904:	c3                   	ret
80100905:	b8 00 01 00 00       	mov    $0x100,%eax
8010090a:	e8 f1 fa ff ff       	call   80100400 <consputc.part.0>
      while(input.e != input.w &&
8010090f:	a1 08 ef 10 80       	mov    0x8010ef08,%eax
80100914:	3b 05 04 ef 10 80    	cmp    0x8010ef04,%eax
8010091a:	74 9f                	je     801008bb <consoleintr+0x1b>
            input.buf[(input.e-1) % INPUT_BUF] != '\n'){
8010091c:	83 e8 01             	sub    $0x1,%eax
8010091f:	89 c2                	mov    %eax,%edx
80100921:	83 e2 7f             	and    $0x7f,%edx
      while(input.e != input.w &&
80100924:	80 ba 80 ee 10 80 0a 	cmpb   $0xa,-0x7fef1180(%edx)
8010092b:	74 8e                	je     801008bb <consoleintr+0x1b>
  if(panicked){
8010092d:	8b 15 58 ef 10 80    	mov    0x8010ef58,%edx
        input.e--;
80100933:	a3 08 ef 10 80       	mov    %eax,0x8010ef08
  if(panicked){
80100938:	85 d2                	test   %edx,%edx
8010093a:	74 c9                	je     80100905 <consoleintr+0x65>
8010093c:	fa                   	cli
    for(;;)
8010093d:	eb fe                	jmp    8010093d <consoleintr+0x9d>
8010093f:	90                   	nop
    switch(c){
80100940:	83 fb 7f             	cmp    $0x7f,%ebx
80100943:	75 2b                	jne    80100970 <consoleintr+0xd0>
      if(input.e != input.w){
80100945:	a1 08 ef 10 80       	mov    0x8010ef08,%eax
8010094a:	3b 05 04 ef 10 80    	cmp    0x8010ef04,%eax
80100950:	0f 84 65 ff ff ff    	je     801008bb <consoleintr+0x1b>
        input.e--;
80100956:	83 e8 01             	sub    $0x1,%eax
80100959:	a3 08 ef 10 80       	mov    %eax,0x8010ef08
  if(panicked){
8010095e:	a1 58 ef 10 80       	mov    0x8010ef58,%eax
80100963:	85 c0                	test   %eax,%eax
80100965:	0f 84 ce 00 00 00    	je     80100a39 <consoleintr+0x199>
8010096b:	fa                   	cli
    for(;;)
8010096c:	eb fe                	jmp    8010096c <consoleintr+0xcc>
8010096e:	66 90                	xchg   %ax,%ax
      if(c != 0 && input.e-input.r < INPUT_BUF){
80100970:	a1 08 ef 10 80       	mov    0x8010ef08,%eax
80100975:	89 c2                	mov    %eax,%edx
80100977:	2b 15 00 ef 10 80    	sub    0x8010ef00,%edx
8010097d:	83 fa 7f             	cmp    $0x7f,%edx
80100980:	0f 87 35 ff ff ff    	ja     801008bb <consoleintr+0x1b>
  if(panicked){
80100986:	8b 0d 58 ef 10 80    	mov    0x8010ef58,%ecx
        input.buf[input.e++ % INPUT_BUF] = c;
8010098c:	8d 50 01             	lea    0x1(%eax),%edx
8010098f:	83 e0 7f             	and    $0x7f,%eax
80100992:	89 15 08 ef 10 80    	mov    %edx,0x8010ef08
80100998:	88 98 80 ee 10 80    	mov    %bl,-0x7fef1180(%eax)
  if(panicked){
8010099e:	85 c9                	test   %ecx,%ecx
801009a0:	0f 85 ae 00 00 00    	jne    80100a54 <consoleintr+0x1b4>
801009a6:	89 d8                	mov    %ebx,%eax
801009a8:	e8 53 fa ff ff       	call   80100400 <consputc.part.0>
        if(c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF){
801009ad:	83 fb 0a             	cmp    $0xa,%ebx
801009b0:	74 68                	je     80100a1a <consoleintr+0x17a>
801009b2:	83 fb 04             	cmp    $0x4,%ebx
801009b5:	74 63                	je     80100a1a <consoleintr+0x17a>
801009b7:	a1 00 ef 10 80       	mov    0x8010ef00,%eax
801009bc:	83 e8 80             	sub    $0xffffff80,%eax
801009bf:	39 05 08 ef 10 80    	cmp    %eax,0x8010ef08
801009c5:	0f 85 f0 fe ff ff    	jne    801008bb <consoleintr+0x1b>
801009cb:	eb 52                	jmp    80100a1f <consoleintr+0x17f>
801009cd:	8d 76 00             	lea    0x0(%esi),%esi
      if(c != 0 && input.e-input.r < INPUT_BUF){
801009d0:	85 db                	test   %ebx,%ebx
801009d2:	0f 84 e3 fe ff ff    	je     801008bb <consoleintr+0x1b>
801009d8:	a1 08 ef 10 80       	mov    0x8010ef08,%eax
801009dd:	89 c2                	mov    %eax,%edx
801009df:	2b 15 00 ef 10 80    	sub    0x8010ef00,%edx
801009e5:	83 fa 7f             	cmp    $0x7f,%edx
801009e8:	0f 87 cd fe ff ff    	ja     801008bb <consoleintr+0x1b>
        input.buf[input.e++ % INPUT_BUF] = c;
801009ee:	8d 50 01             	lea    0x1(%eax),%edx
  if(panicked){
801009f1:	8b 0d 58 ef 10 80    	mov    0x8010ef58,%ecx
        input.buf[input.e++ % INPUT_BUF] = c;
801009f7:	83 e0 7f             	and    $0x7f,%eax
        c = (c == '\r') ? '\n' : c;
801009fa:	83 fb 0d             	cmp    $0xd,%ebx
801009fd:	75 93                	jne    80100992 <consoleintr+0xf2>
        input.buf[input.e++ % INPUT_BUF] = c;
801009ff:	89 15 08 ef 10 80    	mov    %edx,0x8010ef08
80100a05:	c6 80 80 ee 10 80 0a 	movb   $0xa,-0x7fef1180(%eax)
  if(panicked){
80100a0c:	85 c9                	test   %ecx,%ecx
80100a0e:	75 44                	jne    80100a54 <consoleintr+0x1b4>
80100a10:	b8 0a 00 00 00       	mov    $0xa,%eax
80100a15:	e8 e6 f9 ff ff       	call   80100400 <consputc.part.0>
          input.w = input.e;
80100a1a:	a1 08 ef 10 80       	mov    0x8010ef08,%eax
          wakeup(&input.r);
80100a1f:	83 ec 0c             	sub    $0xc,%esp
          input.w = input.e;
80100a22:	a3 04 ef 10 80       	mov    %eax,0x8010ef04
          wakeup(&input.r);
80100a27:	68 00 ef 10 80       	push   $0x8010ef00
80100a2c:	e8 af 36 00 00       	call   801040e0 <wakeup>
80100a31:	83 c4 10             	add    $0x10,%esp
80100a34:	e9 82 fe ff ff       	jmp    801008bb <consoleintr+0x1b>
80100a39:	b8 00 01 00 00       	mov    $0x100,%eax
80100a3e:	e8 bd f9 ff ff       	call   80100400 <consputc.part.0>
80100a43:	e9 73 fe ff ff       	jmp    801008bb <consoleintr+0x1b>
}
80100a48:	8d 65 f4             	lea    -0xc(%ebp),%esp
80100a4b:	5b                   	pop    %ebx
80100a4c:	5e                   	pop    %esi
80100a4d:	5f                   	pop    %edi
80100a4e:	5d                   	pop    %ebp
    procdump();  // now call procdump() wo. cons.lock held
80100a4f:	e9 6c 37 00 00       	jmp    801041c0 <procdump>
80100a54:	fa                   	cli
    for(;;)
80100a55:	eb fe                	jmp    80100a55 <consoleintr+0x1b5>
80100a57:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80100a5e:	00 
80100a5f:	90                   	nop

80100a60 <consoleinit>:

void
consoleinit(void)
{
80100a60:	55                   	push   %ebp
80100a61:	89 e5                	mov    %esp,%ebp
80100a63:	83 ec 10             	sub    $0x10,%esp
  initlock(&cons.lock, "console");
80100a66:	68 c8 71 10 80       	push   $0x801071c8
80100a6b:	68 20 ef 10 80       	push   $0x8010ef20
80100a70:	e8 3b 39 00 00       	call   801043b0 <initlock>

  devsw[CONSOLE].write = consolewrite;
  devsw[CONSOLE].read = consoleread;
  cons.locking = 1;

  ioapicenable(IRQ_KBD, 0);
80100a75:	58                   	pop    %eax
80100a76:	5a                   	pop    %edx
80100a77:	6a 00                	push   $0x0
80100a79:	6a 01                	push   $0x1
  devsw[CONSOLE].write = consolewrite;
80100a7b:	c7 05 0c f9 10 80 b0 	movl   $0x801005b0,0x8010f90c
80100a82:	05 10 80 
  devsw[CONSOLE].read = consoleread;
80100a85:	c7 05 08 f9 10 80 80 	movl   $0x80100280,0x8010f908
80100a8c:	02 10 80 
  cons.locking = 1;
80100a8f:	c7 05 54 ef 10 80 01 	movl   $0x1,0x8010ef54
80100a96:	00 00 00 
  ioapicenable(IRQ_KBD, 0);
80100a99:	e8 c2 19 00 00       	call   80102460 <ioapicenable>
}
80100a9e:	83 c4 10             	add    $0x10,%esp
80100aa1:	c9                   	leave
80100aa2:	c3                   	ret
80100aa3:	66 90                	xchg   %ax,%ax
80100aa5:	66 90                	xchg   %ax,%ax
80100aa7:	66 90                	xchg   %ax,%ax
80100aa9:	66 90                	xchg   %ax,%ax
80100aab:	66 90                	xchg   %ax,%ax
80100aad:	66 90                	xchg   %ax,%ax
80100aaf:	90                   	nop

80100ab0 <exec>:
#include "x86.h"
#include "elf.h"

int
exec(char *path, char **argv)
{
80100ab0:	55                   	push   %ebp
80100ab1:	89 e5                	mov    %esp,%ebp
80100ab3:	57                   	push   %edi
80100ab4:	56                   	push   %esi
80100ab5:	53                   	push   %ebx
80100ab6:	81 ec 0c 01 00 00    	sub    $0x10c,%esp
  uint argc, sz, sp, ustack[3+MAXARG+1];
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;
  struct proc *curproc = myproc();
80100abc:	e8 9f 2e 00 00       	call   80103960 <myproc>
80100ac1:	89 85 ec fe ff ff    	mov    %eax,-0x114(%ebp)

  begin_op();
80100ac7:	e8 74 22 00 00       	call   80102d40 <begin_op>

  if((ip = namei(path)) == 0){
80100acc:	83 ec 0c             	sub    $0xc,%esp
80100acf:	ff 75 08             	push   0x8(%ebp)
80100ad2:	e8 a9 15 00 00       	call   80102080 <namei>
80100ad7:	83 c4 10             	add    $0x10,%esp
80100ada:	85 c0                	test   %eax,%eax
80100adc:	0f 84 30 03 00 00    	je     80100e12 <exec+0x362>
    end_op();
    cprintf("exec: fail\n");
    return -1;
  }
  ilock(ip);
80100ae2:	83 ec 0c             	sub    $0xc,%esp
80100ae5:	89 c7                	mov    %eax,%edi
80100ae7:	50                   	push   %eax
80100ae8:	e8 b3 0c 00 00       	call   801017a0 <ilock>
  pgdir = 0;

  // Check ELF header
  if(readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf))
80100aed:	8d 85 24 ff ff ff    	lea    -0xdc(%ebp),%eax
80100af3:	6a 34                	push   $0x34
80100af5:	6a 00                	push   $0x0
80100af7:	50                   	push   %eax
80100af8:	57                   	push   %edi
80100af9:	e8 b2 0f 00 00       	call   80101ab0 <readi>
80100afe:	83 c4 20             	add    $0x20,%esp
80100b01:	83 f8 34             	cmp    $0x34,%eax
80100b04:	0f 85 01 01 00 00    	jne    80100c0b <exec+0x15b>
    goto bad;
  if(elf.magic != ELF_MAGIC)
80100b0a:	81 bd 24 ff ff ff 7f 	cmpl   $0x464c457f,-0xdc(%ebp)
80100b11:	45 4c 46 
80100b14:	0f 85 f1 00 00 00    	jne    80100c0b <exec+0x15b>
    goto bad;

  if((pgdir = setupkvm()) == 0)
80100b1a:	e8 01 63 00 00       	call   80106e20 <setupkvm>
80100b1f:	89 85 f4 fe ff ff    	mov    %eax,-0x10c(%ebp)
80100b25:	85 c0                	test   %eax,%eax
80100b27:	0f 84 de 00 00 00    	je     80100c0b <exec+0x15b>
    goto bad;

  // Load program into memory.
  sz = 0;
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
80100b2d:	66 83 bd 50 ff ff ff 	cmpw   $0x0,-0xb0(%ebp)
80100b34:	00 
80100b35:	8b b5 40 ff ff ff    	mov    -0xc0(%ebp),%esi
80100b3b:	0f 84 a1 02 00 00    	je     80100de2 <exec+0x332>
  sz = 0;
80100b41:	c7 85 f0 fe ff ff 00 	movl   $0x0,-0x110(%ebp)
80100b48:	00 00 00 
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
80100b4b:	31 db                	xor    %ebx,%ebx
80100b4d:	e9 8c 00 00 00       	jmp    80100bde <exec+0x12e>
80100b52:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
80100b58:	83 bd 04 ff ff ff 01 	cmpl   $0x1,-0xfc(%ebp)
80100b5f:	75 6c                	jne    80100bcd <exec+0x11d>
      continue;
    if(ph.memsz < ph.filesz)
80100b61:	8b 85 18 ff ff ff    	mov    -0xe8(%ebp),%eax
80100b67:	3b 85 14 ff ff ff    	cmp    -0xec(%ebp),%eax
80100b6d:	0f 82 87 00 00 00    	jb     80100bfa <exec+0x14a>
      goto bad;
    if(ph.vaddr + ph.memsz < ph.vaddr)
80100b73:	03 85 0c ff ff ff    	add    -0xf4(%ebp),%eax
80100b79:	72 7f                	jb     80100bfa <exec+0x14a>
      goto bad;
    if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
80100b7b:	83 ec 04             	sub    $0x4,%esp
80100b7e:	50                   	push   %eax
80100b7f:	ff b5 f0 fe ff ff    	push   -0x110(%ebp)
80100b85:	ff b5 f4 fe ff ff    	push   -0x10c(%ebp)
80100b8b:	e8 c0 60 00 00       	call   80106c50 <allocuvm>
80100b90:	83 c4 10             	add    $0x10,%esp
80100b93:	89 85 f0 fe ff ff    	mov    %eax,-0x110(%ebp)
80100b99:	85 c0                	test   %eax,%eax
80100b9b:	74 5d                	je     80100bfa <exec+0x14a>
      goto bad;
    if(ph.vaddr % PGSIZE != 0)
80100b9d:	8b 85 0c ff ff ff    	mov    -0xf4(%ebp),%eax
80100ba3:	a9 ff 0f 00 00       	test   $0xfff,%eax
80100ba8:	75 50                	jne    80100bfa <exec+0x14a>
      goto bad;
    if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
80100baa:	83 ec 0c             	sub    $0xc,%esp
80100bad:	ff b5 14 ff ff ff    	push   -0xec(%ebp)
80100bb3:	ff b5 08 ff ff ff    	push   -0xf8(%ebp)
80100bb9:	57                   	push   %edi
80100bba:	50                   	push   %eax
80100bbb:	ff b5 f4 fe ff ff    	push   -0x10c(%ebp)
80100bc1:	e8 ba 5f 00 00       	call   80106b80 <loaduvm>
80100bc6:	83 c4 20             	add    $0x20,%esp
80100bc9:	85 c0                	test   %eax,%eax
80100bcb:	78 2d                	js     80100bfa <exec+0x14a>
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
80100bcd:	0f b7 85 50 ff ff ff 	movzwl -0xb0(%ebp),%eax
80100bd4:	83 c3 01             	add    $0x1,%ebx
80100bd7:	83 c6 20             	add    $0x20,%esi
80100bda:	39 d8                	cmp    %ebx,%eax
80100bdc:	7e 52                	jle    80100c30 <exec+0x180>
    if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
80100bde:	8d 85 04 ff ff ff    	lea    -0xfc(%ebp),%eax
80100be4:	6a 20                	push   $0x20
80100be6:	56                   	push   %esi
80100be7:	50                   	push   %eax
80100be8:	57                   	push   %edi
80100be9:	e8 c2 0e 00 00       	call   80101ab0 <readi>
80100bee:	83 c4 10             	add    $0x10,%esp
80100bf1:	83 f8 20             	cmp    $0x20,%eax
80100bf4:	0f 84 5e ff ff ff    	je     80100b58 <exec+0xa8>
  freevm(oldpgdir);
  return 0;

 bad:
  if(pgdir)
    freevm(pgdir);
80100bfa:	83 ec 0c             	sub    $0xc,%esp
80100bfd:	ff b5 f4 fe ff ff    	push   -0x10c(%ebp)
80100c03:	e8 98 61 00 00       	call   80106da0 <freevm>
  if(ip){
80100c08:	83 c4 10             	add    $0x10,%esp
    iunlockput(ip);
80100c0b:	83 ec 0c             	sub    $0xc,%esp
80100c0e:	57                   	push   %edi
80100c0f:	e8 1c 0e 00 00       	call   80101a30 <iunlockput>
    end_op();
80100c14:	e8 97 21 00 00       	call   80102db0 <end_op>
80100c19:	83 c4 10             	add    $0x10,%esp
    return -1;
80100c1c:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
  }
  return -1;
}
80100c21:	8d 65 f4             	lea    -0xc(%ebp),%esp
80100c24:	5b                   	pop    %ebx
80100c25:	5e                   	pop    %esi
80100c26:	5f                   	pop    %edi
80100c27:	5d                   	pop    %ebp
80100c28:	c3                   	ret
80100c29:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
  sz = PGROUNDUP(sz);
80100c30:	8b b5 f0 fe ff ff    	mov    -0x110(%ebp),%esi
80100c36:	81 c6 ff 0f 00 00    	add    $0xfff,%esi
80100c3c:	81 e6 00 f0 ff ff    	and    $0xfffff000,%esi
  if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
80100c42:	8d 9e 00 20 00 00    	lea    0x2000(%esi),%ebx
  iunlockput(ip);
80100c48:	83 ec 0c             	sub    $0xc,%esp
80100c4b:	57                   	push   %edi
80100c4c:	e8 df 0d 00 00       	call   80101a30 <iunlockput>
  end_op();
80100c51:	e8 5a 21 00 00       	call   80102db0 <end_op>
  if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
80100c56:	83 c4 0c             	add    $0xc,%esp
80100c59:	53                   	push   %ebx
80100c5a:	56                   	push   %esi
80100c5b:	8b b5 f4 fe ff ff    	mov    -0x10c(%ebp),%esi
80100c61:	56                   	push   %esi
80100c62:	e8 e9 5f 00 00       	call   80106c50 <allocuvm>
80100c67:	83 c4 10             	add    $0x10,%esp
80100c6a:	89 c7                	mov    %eax,%edi
80100c6c:	85 c0                	test   %eax,%eax
80100c6e:	0f 84 86 00 00 00    	je     80100cfa <exec+0x24a>
  clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
80100c74:	83 ec 08             	sub    $0x8,%esp
80100c77:	8d 80 00 e0 ff ff    	lea    -0x2000(%eax),%eax
  sp = sz;
80100c7d:	89 fb                	mov    %edi,%ebx
  clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
80100c7f:	50                   	push   %eax
80100c80:	56                   	push   %esi
  for(argc = 0; argv[argc]; argc++) {
80100c81:	31 f6                	xor    %esi,%esi
  clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
80100c83:	e8 38 62 00 00       	call   80106ec0 <clearpteu>
  for(argc = 0; argv[argc]; argc++) {
80100c88:	8b 45 0c             	mov    0xc(%ebp),%eax
80100c8b:	83 c4 10             	add    $0x10,%esp
80100c8e:	8b 10                	mov    (%eax),%edx
80100c90:	85 d2                	test   %edx,%edx
80100c92:	0f 84 56 01 00 00    	je     80100dee <exec+0x33e>
80100c98:	89 bd f0 fe ff ff    	mov    %edi,-0x110(%ebp)
80100c9e:	8b 7d 0c             	mov    0xc(%ebp),%edi
80100ca1:	eb 23                	jmp    80100cc6 <exec+0x216>
80100ca3:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
80100ca8:	8d 46 01             	lea    0x1(%esi),%eax
    ustack[3+argc] = sp;
80100cab:	89 9c b5 64 ff ff ff 	mov    %ebx,-0x9c(%ebp,%esi,4)
80100cb2:	8d 8d 58 ff ff ff    	lea    -0xa8(%ebp),%ecx
  for(argc = 0; argv[argc]; argc++) {
80100cb8:	8b 14 87             	mov    (%edi,%eax,4),%edx
80100cbb:	85 d2                	test   %edx,%edx
80100cbd:	74 51                	je     80100d10 <exec+0x260>
    if(argc >= MAXARG)
80100cbf:	83 f8 20             	cmp    $0x20,%eax
80100cc2:	74 36                	je     80100cfa <exec+0x24a>
80100cc4:	89 c6                	mov    %eax,%esi
    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
80100cc6:	83 ec 0c             	sub    $0xc,%esp
80100cc9:	52                   	push   %edx
80100cca:	e8 c1 3b 00 00       	call   80104890 <strlen>
80100ccf:	29 c3                	sub    %eax,%ebx
    if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
80100cd1:	58                   	pop    %eax
80100cd2:	ff 34 b7             	push   (%edi,%esi,4)
    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
80100cd5:	83 eb 01             	sub    $0x1,%ebx
80100cd8:	83 e3 fc             	and    $0xfffffffc,%ebx
    if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
80100cdb:	e8 b0 3b 00 00       	call   80104890 <strlen>
80100ce0:	83 c0 01             	add    $0x1,%eax
80100ce3:	50                   	push   %eax
80100ce4:	ff 34 b7             	push   (%edi,%esi,4)
80100ce7:	53                   	push   %ebx
80100ce8:	ff b5 f4 fe ff ff    	push   -0x10c(%ebp)
80100cee:	e8 9d 63 00 00       	call   80107090 <copyout>
80100cf3:	83 c4 20             	add    $0x20,%esp
80100cf6:	85 c0                	test   %eax,%eax
80100cf8:	79 ae                	jns    80100ca8 <exec+0x1f8>
    freevm(pgdir);
80100cfa:	83 ec 0c             	sub    $0xc,%esp
80100cfd:	ff b5 f4 fe ff ff    	push   -0x10c(%ebp)
80100d03:	e8 98 60 00 00       	call   80106da0 <freevm>
80100d08:	83 c4 10             	add    $0x10,%esp
80100d0b:	e9 0c ff ff ff       	jmp    80100c1c <exec+0x16c>
  ustack[2] = sp - (argc+1)*4;  // argv pointer
80100d10:	8d 14 b5 08 00 00 00 	lea    0x8(,%esi,4),%edx
  ustack[3+argc] = 0;
80100d17:	8b bd f0 fe ff ff    	mov    -0x110(%ebp),%edi
80100d1d:	89 85 f0 fe ff ff    	mov    %eax,-0x110(%ebp)
80100d23:	8d 46 04             	lea    0x4(%esi),%eax
  sp -= (3+argc+1) * 4;
80100d26:	8d 72 0c             	lea    0xc(%edx),%esi
  ustack[3+argc] = 0;
80100d29:	c7 84 85 58 ff ff ff 	movl   $0x0,-0xa8(%ebp,%eax,4)
80100d30:	00 00 00 00 
  ustack[1] = argc;
80100d34:	8b 85 f0 fe ff ff    	mov    -0x110(%ebp),%eax
  ustack[0] = 0xffffffff;  // fake return PC
80100d3a:	c7 85 58 ff ff ff ff 	movl   $0xffffffff,-0xa8(%ebp)
80100d41:	ff ff ff 
  ustack[1] = argc;
80100d44:	89 85 5c ff ff ff    	mov    %eax,-0xa4(%ebp)
  ustack[2] = sp - (argc+1)*4;  // argv pointer
80100d4a:	89 d8                	mov    %ebx,%eax
  sp -= (3+argc+1) * 4;
80100d4c:	29 f3                	sub    %esi,%ebx
  ustack[2] = sp - (argc+1)*4;  // argv pointer
80100d4e:	29 d0                	sub    %edx,%eax
80100d50:	89 85 60 ff ff ff    	mov    %eax,-0xa0(%ebp)
  if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
80100d56:	56                   	push   %esi
80100d57:	51                   	push   %ecx
80100d58:	53                   	push   %ebx
80100d59:	ff b5 f4 fe ff ff    	push   -0x10c(%ebp)
80100d5f:	e8 2c 63 00 00       	call   80107090 <copyout>
80100d64:	83 c4 10             	add    $0x10,%esp
80100d67:	85 c0                	test   %eax,%eax
80100d69:	78 8f                	js     80100cfa <exec+0x24a>
  for(last=s=path; *s; s++)
80100d6b:	8b 45 08             	mov    0x8(%ebp),%eax
80100d6e:	8b 55 08             	mov    0x8(%ebp),%edx
80100d71:	0f b6 00             	movzbl (%eax),%eax
80100d74:	84 c0                	test   %al,%al
80100d76:	74 17                	je     80100d8f <exec+0x2df>
80100d78:	89 d1                	mov    %edx,%ecx
80100d7a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
      last = s+1;
80100d80:	83 c1 01             	add    $0x1,%ecx
80100d83:	3c 2f                	cmp    $0x2f,%al
  for(last=s=path; *s; s++)
80100d85:	0f b6 01             	movzbl (%ecx),%eax
      last = s+1;
80100d88:	0f 44 d1             	cmove  %ecx,%edx
  for(last=s=path; *s; s++)
80100d8b:	84 c0                	test   %al,%al
80100d8d:	75 f1                	jne    80100d80 <exec+0x2d0>
  safestrcpy(curproc->name, last, sizeof(curproc->name));
80100d8f:	83 ec 04             	sub    $0x4,%esp
80100d92:	6a 10                	push   $0x10
80100d94:	52                   	push   %edx
80100d95:	8b b5 ec fe ff ff    	mov    -0x114(%ebp),%esi
80100d9b:	8d 46 6c             	lea    0x6c(%esi),%eax
80100d9e:	50                   	push   %eax
80100d9f:	e8 ac 3a 00 00       	call   80104850 <safestrcpy>
  curproc->pgdir = pgdir;
80100da4:	8b 8d f4 fe ff ff    	mov    -0x10c(%ebp),%ecx
  oldpgdir = curproc->pgdir;
80100daa:	89 f0                	mov    %esi,%eax
80100dac:	8b 76 04             	mov    0x4(%esi),%esi
  curproc->sz = sz;
80100daf:	89 38                	mov    %edi,(%eax)
  curproc->pgdir = pgdir;
80100db1:	89 48 04             	mov    %ecx,0x4(%eax)
  curproc->tf->eip = elf.entry;  // main
80100db4:	89 c1                	mov    %eax,%ecx
80100db6:	8b 95 3c ff ff ff    	mov    -0xc4(%ebp),%edx
80100dbc:	8b 40 18             	mov    0x18(%eax),%eax
80100dbf:	89 50 38             	mov    %edx,0x38(%eax)
  curproc->tf->esp = sp;
80100dc2:	8b 41 18             	mov    0x18(%ecx),%eax
80100dc5:	89 58 44             	mov    %ebx,0x44(%eax)
  switchuvm(curproc);
80100dc8:	89 0c 24             	mov    %ecx,(%esp)
80100dcb:	e8 20 5c 00 00       	call   801069f0 <switchuvm>
  freevm(oldpgdir);
80100dd0:	89 34 24             	mov    %esi,(%esp)
80100dd3:	e8 c8 5f 00 00       	call   80106da0 <freevm>
  return 0;
80100dd8:	83 c4 10             	add    $0x10,%esp
80100ddb:	31 c0                	xor    %eax,%eax
80100ddd:	e9 3f fe ff ff       	jmp    80100c21 <exec+0x171>
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
80100de2:	bb 00 20 00 00       	mov    $0x2000,%ebx
80100de7:	31 f6                	xor    %esi,%esi
80100de9:	e9 5a fe ff ff       	jmp    80100c48 <exec+0x198>
  for(argc = 0; argv[argc]; argc++) {
80100dee:	be 10 00 00 00       	mov    $0x10,%esi
80100df3:	ba 04 00 00 00       	mov    $0x4,%edx
80100df8:	b8 03 00 00 00       	mov    $0x3,%eax
80100dfd:	c7 85 f0 fe ff ff 00 	movl   $0x0,-0x110(%ebp)
80100e04:	00 00 00 
80100e07:	8d 8d 58 ff ff ff    	lea    -0xa8(%ebp),%ecx
80100e0d:	e9 17 ff ff ff       	jmp    80100d29 <exec+0x279>
    end_op();
80100e12:	e8 99 1f 00 00       	call   80102db0 <end_op>
    cprintf("exec: fail\n");
80100e17:	83 ec 0c             	sub    $0xc,%esp
80100e1a:	68 d0 71 10 80       	push   $0x801071d0
80100e1f:	e8 8c f8 ff ff       	call   801006b0 <cprintf>
    return -1;
80100e24:	83 c4 10             	add    $0x10,%esp
80100e27:	e9 f0 fd ff ff       	jmp    80100c1c <exec+0x16c>
80100e2c:	66 90                	xchg   %ax,%ax
80100e2e:	66 90                	xchg   %ax,%ax

80100e30 <fileinit>:
  struct file file[NFILE];
} ftable;

void
fileinit(void)
{
80100e30:	55                   	push   %ebp
80100e31:	89 e5                	mov    %esp,%ebp
80100e33:	83 ec 10             	sub    $0x10,%esp
  initlock(&ftable.lock, "ftable");
80100e36:	68 dc 71 10 80       	push   $0x801071dc
80100e3b:	68 60 ef 10 80       	push   $0x8010ef60
80100e40:	e8 6b 35 00 00       	call   801043b0 <initlock>
}
80100e45:	83 c4 10             	add    $0x10,%esp
80100e48:	c9                   	leave
80100e49:	c3                   	ret
80100e4a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi

80100e50 <filealloc>:

// Allocate a file structure.
struct file*
filealloc(void)
{
80100e50:	55                   	push   %ebp
80100e51:	89 e5                	mov    %esp,%ebp
80100e53:	53                   	push   %ebx
  struct file *f;

  acquire(&ftable.lock);
  for(f = ftable.file; f < ftable.file + NFILE; f++){
80100e54:	bb 94 ef 10 80       	mov    $0x8010ef94,%ebx
{
80100e59:	83 ec 10             	sub    $0x10,%esp
  acquire(&ftable.lock);
80100e5c:	68 60 ef 10 80       	push   $0x8010ef60
80100e61:	e8 3a 37 00 00       	call   801045a0 <acquire>
80100e66:	83 c4 10             	add    $0x10,%esp
80100e69:	eb 10                	jmp    80100e7b <filealloc+0x2b>
80100e6b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
  for(f = ftable.file; f < ftable.file + NFILE; f++){
80100e70:	83 c3 18             	add    $0x18,%ebx
80100e73:	81 fb f4 f8 10 80    	cmp    $0x8010f8f4,%ebx
80100e79:	74 25                	je     80100ea0 <filealloc+0x50>
    if(f->ref == 0){
80100e7b:	8b 43 04             	mov    0x4(%ebx),%eax
80100e7e:	85 c0                	test   %eax,%eax
80100e80:	75 ee                	jne    80100e70 <filealloc+0x20>
      f->ref = 1;
      release(&ftable.lock);
80100e82:	83 ec 0c             	sub    $0xc,%esp
      f->ref = 1;
80100e85:	c7 43 04 01 00 00 00 	movl   $0x1,0x4(%ebx)
      release(&ftable.lock);
80100e8c:	68 60 ef 10 80       	push   $0x8010ef60
80100e91:	e8 aa 36 00 00       	call   80104540 <release>
      return f;
    }
  }
  release(&ftable.lock);
  return 0;
}
80100e96:	89 d8                	mov    %ebx,%eax
      return f;
80100e98:	83 c4 10             	add    $0x10,%esp
}
80100e9b:	8b 5d fc             	mov    -0x4(%ebp),%ebx
80100e9e:	c9                   	leave
80100e9f:	c3                   	ret
  release(&ftable.lock);
80100ea0:	83 ec 0c             	sub    $0xc,%esp
  return 0;
80100ea3:	31 db                	xor    %ebx,%ebx
  release(&ftable.lock);
80100ea5:	68 60 ef 10 80       	push   $0x8010ef60
80100eaa:	e8 91 36 00 00       	call   80104540 <release>
}
80100eaf:	89 d8                	mov    %ebx,%eax
  return 0;
80100eb1:	83 c4 10             	add    $0x10,%esp
}
80100eb4:	8b 5d fc             	mov    -0x4(%ebp),%ebx
80100eb7:	c9                   	leave
80100eb8:	c3                   	ret
80100eb9:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi

80100ec0 <filedup>:

// Increment ref count for file f.
struct file*
filedup(struct file *f)
{
80100ec0:	55                   	push   %ebp
80100ec1:	89 e5                	mov    %esp,%ebp
80100ec3:	53                   	push   %ebx
80100ec4:	83 ec 10             	sub    $0x10,%esp
80100ec7:	8b 5d 08             	mov    0x8(%ebp),%ebx
  acquire(&ftable.lock);
80100eca:	68 60 ef 10 80       	push   $0x8010ef60
80100ecf:	e8 cc 36 00 00       	call   801045a0 <acquire>
  if(f->ref < 1)
80100ed4:	8b 43 04             	mov    0x4(%ebx),%eax
80100ed7:	83 c4 10             	add    $0x10,%esp
80100eda:	85 c0                	test   %eax,%eax
80100edc:	7e 1a                	jle    80100ef8 <filedup+0x38>
    panic("filedup");
  f->ref++;
80100ede:	83 c0 01             	add    $0x1,%eax
  release(&ftable.lock);
80100ee1:	83 ec 0c             	sub    $0xc,%esp
  f->ref++;
80100ee4:	89 43 04             	mov    %eax,0x4(%ebx)
  release(&ftable.lock);
80100ee7:	68 60 ef 10 80       	push   $0x8010ef60
80100eec:	e8 4f 36 00 00       	call   80104540 <release>
  return f;
}
80100ef1:	89 d8                	mov    %ebx,%eax
80100ef3:	8b 5d fc             	mov    -0x4(%ebp),%ebx
80100ef6:	c9                   	leave
80100ef7:	c3                   	ret
    panic("filedup");
80100ef8:	83 ec 0c             	sub    $0xc,%esp
80100efb:	68 e3 71 10 80       	push   $0x801071e3
80100f00:	e8 7b f4 ff ff       	call   80100380 <panic>
80100f05:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80100f0c:	00 
80100f0d:	8d 76 00             	lea    0x0(%esi),%esi

80100f10 <fileclose>:

// Close file f.  (Decrement ref count, close when reaches 0.)
void
fileclose(struct file *f)
{
80100f10:	55                   	push   %ebp
80100f11:	89 e5                	mov    %esp,%ebp
80100f13:	57                   	push   %edi
80100f14:	56                   	push   %esi
80100f15:	53                   	push   %ebx
80100f16:	83 ec 28             	sub    $0x28,%esp
80100f19:	8b 5d 08             	mov    0x8(%ebp),%ebx
  struct file ff;

  acquire(&ftable.lock);
80100f1c:	68 60 ef 10 80       	push   $0x8010ef60
80100f21:	e8 7a 36 00 00       	call   801045a0 <acquire>
  if(f->ref < 1)
80100f26:	8b 53 04             	mov    0x4(%ebx),%edx
80100f29:	83 c4 10             	add    $0x10,%esp
80100f2c:	85 d2                	test   %edx,%edx
80100f2e:	0f 8e a5 00 00 00    	jle    80100fd9 <fileclose+0xc9>
    panic("fileclose");
  if(--f->ref > 0){
80100f34:	83 ea 01             	sub    $0x1,%edx
80100f37:	89 53 04             	mov    %edx,0x4(%ebx)
80100f3a:	75 44                	jne    80100f80 <fileclose+0x70>
    release(&ftable.lock);
    return;
  }
  ff = *f;
80100f3c:	0f b6 43 09          	movzbl 0x9(%ebx),%eax
  f->ref = 0;
  f->type = FD_NONE;
  release(&ftable.lock);
80100f40:	83 ec 0c             	sub    $0xc,%esp
  ff = *f;
80100f43:	8b 3b                	mov    (%ebx),%edi
  f->type = FD_NONE;
80100f45:	c7 03 00 00 00 00    	movl   $0x0,(%ebx)
  ff = *f;
80100f4b:	8b 73 0c             	mov    0xc(%ebx),%esi
80100f4e:	88 45 e7             	mov    %al,-0x19(%ebp)
80100f51:	8b 43 10             	mov    0x10(%ebx),%eax
80100f54:	89 45 e0             	mov    %eax,-0x20(%ebp)
  release(&ftable.lock);
80100f57:	68 60 ef 10 80       	push   $0x8010ef60
80100f5c:	e8 df 35 00 00       	call   80104540 <release>

  if(ff.type == FD_PIPE)
80100f61:	83 c4 10             	add    $0x10,%esp
80100f64:	83 ff 01             	cmp    $0x1,%edi
80100f67:	74 57                	je     80100fc0 <fileclose+0xb0>
    pipeclose(ff.pipe, ff.writable);
  else if(ff.type == FD_INODE){
80100f69:	83 ff 02             	cmp    $0x2,%edi
80100f6c:	74 2a                	je     80100f98 <fileclose+0x88>
    begin_op();
    iput(ff.ip);
    end_op();
  }
}
80100f6e:	8d 65 f4             	lea    -0xc(%ebp),%esp
80100f71:	5b                   	pop    %ebx
80100f72:	5e                   	pop    %esi
80100f73:	5f                   	pop    %edi
80100f74:	5d                   	pop    %ebp
80100f75:	c3                   	ret
80100f76:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80100f7d:	00 
80100f7e:	66 90                	xchg   %ax,%ax
    release(&ftable.lock);
80100f80:	c7 45 08 60 ef 10 80 	movl   $0x8010ef60,0x8(%ebp)
}
80100f87:	8d 65 f4             	lea    -0xc(%ebp),%esp
80100f8a:	5b                   	pop    %ebx
80100f8b:	5e                   	pop    %esi
80100f8c:	5f                   	pop    %edi
80100f8d:	5d                   	pop    %ebp
    release(&ftable.lock);
80100f8e:	e9 ad 35 00 00       	jmp    80104540 <release>
80100f93:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
    begin_op();
80100f98:	e8 a3 1d 00 00       	call   80102d40 <begin_op>
    iput(ff.ip);
80100f9d:	83 ec 0c             	sub    $0xc,%esp
80100fa0:	ff 75 e0             	push   -0x20(%ebp)
80100fa3:	e8 28 09 00 00       	call   801018d0 <iput>
    end_op();
80100fa8:	83 c4 10             	add    $0x10,%esp
}
80100fab:	8d 65 f4             	lea    -0xc(%ebp),%esp
80100fae:	5b                   	pop    %ebx
80100faf:	5e                   	pop    %esi
80100fb0:	5f                   	pop    %edi
80100fb1:	5d                   	pop    %ebp
    end_op();
80100fb2:	e9 f9 1d 00 00       	jmp    80102db0 <end_op>
80100fb7:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80100fbe:	00 
80100fbf:	90                   	nop
    pipeclose(ff.pipe, ff.writable);
80100fc0:	0f be 5d e7          	movsbl -0x19(%ebp),%ebx
80100fc4:	83 ec 08             	sub    $0x8,%esp
80100fc7:	53                   	push   %ebx
80100fc8:	56                   	push   %esi
80100fc9:	e8 32 25 00 00       	call   80103500 <pipeclose>
80100fce:	83 c4 10             	add    $0x10,%esp
}
80100fd1:	8d 65 f4             	lea    -0xc(%ebp),%esp
80100fd4:	5b                   	pop    %ebx
80100fd5:	5e                   	pop    %esi
80100fd6:	5f                   	pop    %edi
80100fd7:	5d                   	pop    %ebp
80100fd8:	c3                   	ret
    panic("fileclose");
80100fd9:	83 ec 0c             	sub    $0xc,%esp
80100fdc:	68 eb 71 10 80       	push   $0x801071eb
80100fe1:	e8 9a f3 ff ff       	call   80100380 <panic>
80100fe6:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80100fed:	00 
80100fee:	66 90                	xchg   %ax,%ax

80100ff0 <filestat>:

// Get metadata about file f.
int
filestat(struct file *f, struct stat *st)
{
80100ff0:	55                   	push   %ebp
80100ff1:	89 e5                	mov    %esp,%ebp
80100ff3:	53                   	push   %ebx
80100ff4:	83 ec 04             	sub    $0x4,%esp
80100ff7:	8b 5d 08             	mov    0x8(%ebp),%ebx
  if(f->type == FD_INODE){
80100ffa:	83 3b 02             	cmpl   $0x2,(%ebx)
80100ffd:	75 31                	jne    80101030 <filestat+0x40>
    ilock(f->ip);
80100fff:	83 ec 0c             	sub    $0xc,%esp
80101002:	ff 73 10             	push   0x10(%ebx)
80101005:	e8 96 07 00 00       	call   801017a0 <ilock>
    stati(f->ip, st);
8010100a:	58                   	pop    %eax
8010100b:	5a                   	pop    %edx
8010100c:	ff 75 0c             	push   0xc(%ebp)
8010100f:	ff 73 10             	push   0x10(%ebx)
80101012:	e8 69 0a 00 00       	call   80101a80 <stati>
    iunlock(f->ip);
80101017:	59                   	pop    %ecx
80101018:	ff 73 10             	push   0x10(%ebx)
8010101b:	e8 60 08 00 00       	call   80101880 <iunlock>
    return 0;
  }
  return -1;
}
80101020:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    return 0;
80101023:	83 c4 10             	add    $0x10,%esp
80101026:	31 c0                	xor    %eax,%eax
}
80101028:	c9                   	leave
80101029:	c3                   	ret
8010102a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
80101030:	8b 5d fc             	mov    -0x4(%ebp),%ebx
  return -1;
80101033:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
}
80101038:	c9                   	leave
80101039:	c3                   	ret
8010103a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi

80101040 <fileread>:

// Read from file f.
int
fileread(struct file *f, char *addr, int n)
{
80101040:	55                   	push   %ebp
80101041:	89 e5                	mov    %esp,%ebp
80101043:	57                   	push   %edi
80101044:	56                   	push   %esi
80101045:	53                   	push   %ebx
80101046:	83 ec 0c             	sub    $0xc,%esp
80101049:	8b 5d 08             	mov    0x8(%ebp),%ebx
8010104c:	8b 75 0c             	mov    0xc(%ebp),%esi
8010104f:	8b 7d 10             	mov    0x10(%ebp),%edi
  int r;

  if(f->readable == 0)
80101052:	80 7b 08 00          	cmpb   $0x0,0x8(%ebx)
80101056:	74 60                	je     801010b8 <fileread+0x78>
    return -1;
  if(f->type == FD_PIPE)
80101058:	8b 03                	mov    (%ebx),%eax
8010105a:	83 f8 01             	cmp    $0x1,%eax
8010105d:	74 41                	je     801010a0 <fileread+0x60>
    return piperead(f->pipe, addr, n);
  if(f->type == FD_INODE){
8010105f:	83 f8 02             	cmp    $0x2,%eax
80101062:	75 5b                	jne    801010bf <fileread+0x7f>
    ilock(f->ip);
80101064:	83 ec 0c             	sub    $0xc,%esp
80101067:	ff 73 10             	push   0x10(%ebx)
8010106a:	e8 31 07 00 00       	call   801017a0 <ilock>
    if((r = readi(f->ip, addr, f->off, n)) > 0)
8010106f:	57                   	push   %edi
80101070:	ff 73 14             	push   0x14(%ebx)
80101073:	56                   	push   %esi
80101074:	ff 73 10             	push   0x10(%ebx)
80101077:	e8 34 0a 00 00       	call   80101ab0 <readi>
8010107c:	83 c4 20             	add    $0x20,%esp
8010107f:	89 c6                	mov    %eax,%esi
80101081:	85 c0                	test   %eax,%eax
80101083:	7e 03                	jle    80101088 <fileread+0x48>
      f->off += r;
80101085:	01 43 14             	add    %eax,0x14(%ebx)
    iunlock(f->ip);
80101088:	83 ec 0c             	sub    $0xc,%esp
8010108b:	ff 73 10             	push   0x10(%ebx)
8010108e:	e8 ed 07 00 00       	call   80101880 <iunlock>
    return r;
80101093:	83 c4 10             	add    $0x10,%esp
  }
  panic("fileread");
}
80101096:	8d 65 f4             	lea    -0xc(%ebp),%esp
80101099:	89 f0                	mov    %esi,%eax
8010109b:	5b                   	pop    %ebx
8010109c:	5e                   	pop    %esi
8010109d:	5f                   	pop    %edi
8010109e:	5d                   	pop    %ebp
8010109f:	c3                   	ret
    return piperead(f->pipe, addr, n);
801010a0:	8b 43 0c             	mov    0xc(%ebx),%eax
801010a3:	89 45 08             	mov    %eax,0x8(%ebp)
}
801010a6:	8d 65 f4             	lea    -0xc(%ebp),%esp
801010a9:	5b                   	pop    %ebx
801010aa:	5e                   	pop    %esi
801010ab:	5f                   	pop    %edi
801010ac:	5d                   	pop    %ebp
    return piperead(f->pipe, addr, n);
801010ad:	e9 0e 26 00 00       	jmp    801036c0 <piperead>
801010b2:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    return -1;
801010b8:	be ff ff ff ff       	mov    $0xffffffff,%esi
801010bd:	eb d7                	jmp    80101096 <fileread+0x56>
  panic("fileread");
801010bf:	83 ec 0c             	sub    $0xc,%esp
801010c2:	68 f5 71 10 80       	push   $0x801071f5
801010c7:	e8 b4 f2 ff ff       	call   80100380 <panic>
801010cc:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi

801010d0 <filewrite>:

//PAGEBREAK!
// Write to file f.
int
filewrite(struct file *f, char *addr, int n)
{
801010d0:	55                   	push   %ebp
801010d1:	89 e5                	mov    %esp,%ebp
801010d3:	57                   	push   %edi
801010d4:	56                   	push   %esi
801010d5:	53                   	push   %ebx
801010d6:	83 ec 1c             	sub    $0x1c,%esp
801010d9:	8b 45 0c             	mov    0xc(%ebp),%eax
801010dc:	8b 5d 08             	mov    0x8(%ebp),%ebx
801010df:	89 45 dc             	mov    %eax,-0x24(%ebp)
801010e2:	8b 45 10             	mov    0x10(%ebp),%eax
  int r;

  if(f->writable == 0)
801010e5:	80 7b 09 00          	cmpb   $0x0,0x9(%ebx)
{
801010e9:	89 45 e4             	mov    %eax,-0x1c(%ebp)
  if(f->writable == 0)
801010ec:	0f 84 bb 00 00 00    	je     801011ad <filewrite+0xdd>
    return -1;
  if(f->type == FD_PIPE)
801010f2:	8b 03                	mov    (%ebx),%eax
801010f4:	83 f8 01             	cmp    $0x1,%eax
801010f7:	0f 84 bf 00 00 00    	je     801011bc <filewrite+0xec>
    return pipewrite(f->pipe, addr, n);
  if(f->type == FD_INODE){
801010fd:	83 f8 02             	cmp    $0x2,%eax
80101100:	0f 85 c8 00 00 00    	jne    801011ce <filewrite+0xfe>
    // and 2 blocks of slop for non-aligned writes.
    // this really belongs lower down, since writei()
    // might be writing a device like the console.
    int max = ((MAXOPBLOCKS-1-1-2) / 2) * 512;
    int i = 0;
    while(i < n){
80101106:	8b 45 e4             	mov    -0x1c(%ebp),%eax
    int i = 0;
80101109:	31 f6                	xor    %esi,%esi
    while(i < n){
8010110b:	85 c0                	test   %eax,%eax
8010110d:	7f 30                	jg     8010113f <filewrite+0x6f>
8010110f:	e9 94 00 00 00       	jmp    801011a8 <filewrite+0xd8>
80101114:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
        n1 = max;

      begin_op();
      ilock(f->ip);
      if ((r = writei(f->ip, addr + i, f->off, n1)) > 0)
        f->off += r;
80101118:	01 43 14             	add    %eax,0x14(%ebx)
      iunlock(f->ip);
8010111b:	83 ec 0c             	sub    $0xc,%esp
        f->off += r;
8010111e:	89 45 e0             	mov    %eax,-0x20(%ebp)
      iunlock(f->ip);
80101121:	ff 73 10             	push   0x10(%ebx)
80101124:	e8 57 07 00 00       	call   80101880 <iunlock>
      end_op();
80101129:	e8 82 1c 00 00       	call   80102db0 <end_op>

      if(r < 0)
        break;
      if(r != n1)
8010112e:	8b 45 e0             	mov    -0x20(%ebp),%eax
80101131:	83 c4 10             	add    $0x10,%esp
80101134:	39 c7                	cmp    %eax,%edi
80101136:	75 5c                	jne    80101194 <filewrite+0xc4>
        panic("short filewrite");
      i += r;
80101138:	01 fe                	add    %edi,%esi
    while(i < n){
8010113a:	39 75 e4             	cmp    %esi,-0x1c(%ebp)
8010113d:	7e 69                	jle    801011a8 <filewrite+0xd8>
      int n1 = n - i;
8010113f:	8b 7d e4             	mov    -0x1c(%ebp),%edi
      if(n1 > max)
80101142:	b8 00 06 00 00       	mov    $0x600,%eax
      int n1 = n - i;
80101147:	29 f7                	sub    %esi,%edi
      if(n1 > max)
80101149:	39 c7                	cmp    %eax,%edi
8010114b:	0f 4f f8             	cmovg  %eax,%edi
      begin_op();
8010114e:	e8 ed 1b 00 00       	call   80102d40 <begin_op>
      ilock(f->ip);
80101153:	83 ec 0c             	sub    $0xc,%esp
80101156:	ff 73 10             	push   0x10(%ebx)
80101159:	e8 42 06 00 00       	call   801017a0 <ilock>
      if ((r = writei(f->ip, addr + i, f->off, n1)) > 0)
8010115e:	57                   	push   %edi
8010115f:	ff 73 14             	push   0x14(%ebx)
80101162:	8b 45 dc             	mov    -0x24(%ebp),%eax
80101165:	01 f0                	add    %esi,%eax
80101167:	50                   	push   %eax
80101168:	ff 73 10             	push   0x10(%ebx)
8010116b:	e8 40 0a 00 00       	call   80101bb0 <writei>
80101170:	83 c4 20             	add    $0x20,%esp
80101173:	85 c0                	test   %eax,%eax
80101175:	7f a1                	jg     80101118 <filewrite+0x48>
80101177:	89 45 e0             	mov    %eax,-0x20(%ebp)
      iunlock(f->ip);
8010117a:	83 ec 0c             	sub    $0xc,%esp
8010117d:	ff 73 10             	push   0x10(%ebx)
80101180:	e8 fb 06 00 00       	call   80101880 <iunlock>
      end_op();
80101185:	e8 26 1c 00 00       	call   80102db0 <end_op>
      if(r < 0)
8010118a:	8b 45 e0             	mov    -0x20(%ebp),%eax
8010118d:	83 c4 10             	add    $0x10,%esp
80101190:	85 c0                	test   %eax,%eax
80101192:	75 14                	jne    801011a8 <filewrite+0xd8>
        panic("short filewrite");
80101194:	83 ec 0c             	sub    $0xc,%esp
80101197:	68 fe 71 10 80       	push   $0x801071fe
8010119c:	e8 df f1 ff ff       	call   80100380 <panic>
801011a1:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    }
    return i == n ? n : -1;
801011a8:	39 75 e4             	cmp    %esi,-0x1c(%ebp)
801011ab:	74 05                	je     801011b2 <filewrite+0xe2>
801011ad:	be ff ff ff ff       	mov    $0xffffffff,%esi
  }
  panic("filewrite");
}
801011b2:	8d 65 f4             	lea    -0xc(%ebp),%esp
801011b5:	89 f0                	mov    %esi,%eax
801011b7:	5b                   	pop    %ebx
801011b8:	5e                   	pop    %esi
801011b9:	5f                   	pop    %edi
801011ba:	5d                   	pop    %ebp
801011bb:	c3                   	ret
    return pipewrite(f->pipe, addr, n);
801011bc:	8b 43 0c             	mov    0xc(%ebx),%eax
801011bf:	89 45 08             	mov    %eax,0x8(%ebp)
}
801011c2:	8d 65 f4             	lea    -0xc(%ebp),%esp
801011c5:	5b                   	pop    %ebx
801011c6:	5e                   	pop    %esi
801011c7:	5f                   	pop    %edi
801011c8:	5d                   	pop    %ebp
    return pipewrite(f->pipe, addr, n);
801011c9:	e9 d2 23 00 00       	jmp    801035a0 <pipewrite>
  panic("filewrite");
801011ce:	83 ec 0c             	sub    $0xc,%esp
801011d1:	68 04 72 10 80       	push   $0x80107204
801011d6:	e8 a5 f1 ff ff       	call   80100380 <panic>
801011db:	66 90                	xchg   %ax,%ax
801011dd:	66 90                	xchg   %ax,%ax
801011df:	90                   	nop

801011e0 <balloc>:
// Blocks.

// Allocate a zeroed disk block.
static uint
balloc(uint dev)
{
801011e0:	55                   	push   %ebp
801011e1:	89 e5                	mov    %esp,%ebp
801011e3:	57                   	push   %edi
801011e4:	56                   	push   %esi
801011e5:	53                   	push   %ebx
801011e6:	83 ec 1c             	sub    $0x1c,%esp
  int b, bi, m;
  struct buf *bp;

  bp = 0;
  for(b = 0; b < sb.size; b += BPB){
801011e9:	8b 0d b4 15 11 80    	mov    0x801115b4,%ecx
{
801011ef:	89 45 dc             	mov    %eax,-0x24(%ebp)
  for(b = 0; b < sb.size; b += BPB){
801011f2:	85 c9                	test   %ecx,%ecx
801011f4:	0f 84 8c 00 00 00    	je     80101286 <balloc+0xa6>
801011fa:	31 ff                	xor    %edi,%edi
    bp = bread(dev, BBLOCK(b, sb));
801011fc:	89 f8                	mov    %edi,%eax
801011fe:	83 ec 08             	sub    $0x8,%esp
80101201:	89 fe                	mov    %edi,%esi
80101203:	c1 f8 0c             	sar    $0xc,%eax
80101206:	03 05 cc 15 11 80    	add    0x801115cc,%eax
8010120c:	50                   	push   %eax
8010120d:	ff 75 dc             	push   -0x24(%ebp)
80101210:	e8 bb ee ff ff       	call   801000d0 <bread>
80101215:	83 c4 10             	add    $0x10,%esp
80101218:	89 7d d8             	mov    %edi,-0x28(%ebp)
8010121b:	89 45 e4             	mov    %eax,-0x1c(%ebp)
    for(bi = 0; bi < BPB && b + bi < sb.size; bi++){
8010121e:	a1 b4 15 11 80       	mov    0x801115b4,%eax
80101223:	89 45 e0             	mov    %eax,-0x20(%ebp)
80101226:	31 c0                	xor    %eax,%eax
80101228:	eb 32                	jmp    8010125c <balloc+0x7c>
8010122a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
      m = 1 << (bi % 8);
80101230:	89 c1                	mov    %eax,%ecx
80101232:	bb 01 00 00 00       	mov    $0x1,%ebx
      if((bp->data[bi/8] & m) == 0){  // Is block free?
80101237:	8b 7d e4             	mov    -0x1c(%ebp),%edi
      m = 1 << (bi % 8);
8010123a:	83 e1 07             	and    $0x7,%ecx
8010123d:	d3 e3                	shl    %cl,%ebx
      if((bp->data[bi/8] & m) == 0){  // Is block free?
8010123f:	89 c1                	mov    %eax,%ecx
80101241:	c1 f9 03             	sar    $0x3,%ecx
80101244:	0f b6 7c 0f 5c       	movzbl 0x5c(%edi,%ecx,1),%edi
80101249:	89 fa                	mov    %edi,%edx
8010124b:	85 df                	test   %ebx,%edi
8010124d:	74 49                	je     80101298 <balloc+0xb8>
    for(bi = 0; bi < BPB && b + bi < sb.size; bi++){
8010124f:	83 c0 01             	add    $0x1,%eax
80101252:	83 c6 01             	add    $0x1,%esi
80101255:	3d 00 10 00 00       	cmp    $0x1000,%eax
8010125a:	74 07                	je     80101263 <balloc+0x83>
8010125c:	8b 55 e0             	mov    -0x20(%ebp),%edx
8010125f:	39 d6                	cmp    %edx,%esi
80101261:	72 cd                	jb     80101230 <balloc+0x50>
        brelse(bp);
        bzero(dev, b + bi);
        return b + bi;
      }
    }
    brelse(bp);
80101263:	8b 7d d8             	mov    -0x28(%ebp),%edi
80101266:	83 ec 0c             	sub    $0xc,%esp
80101269:	ff 75 e4             	push   -0x1c(%ebp)
  for(b = 0; b < sb.size; b += BPB){
8010126c:	81 c7 00 10 00 00    	add    $0x1000,%edi
    brelse(bp);
80101272:	e8 79 ef ff ff       	call   801001f0 <brelse>
  for(b = 0; b < sb.size; b += BPB){
80101277:	83 c4 10             	add    $0x10,%esp
8010127a:	3b 3d b4 15 11 80    	cmp    0x801115b4,%edi
80101280:	0f 82 76 ff ff ff    	jb     801011fc <balloc+0x1c>
  }
  panic("balloc: out of blocks");
80101286:	83 ec 0c             	sub    $0xc,%esp
80101289:	68 0e 72 10 80       	push   $0x8010720e
8010128e:	e8 ed f0 ff ff       	call   80100380 <panic>
80101293:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
        bp->data[bi/8] |= m;  // Mark block in use.
80101298:	8b 7d e4             	mov    -0x1c(%ebp),%edi
        log_write(bp);
8010129b:	83 ec 0c             	sub    $0xc,%esp
        bp->data[bi/8] |= m;  // Mark block in use.
8010129e:	09 da                	or     %ebx,%edx
801012a0:	88 54 0f 5c          	mov    %dl,0x5c(%edi,%ecx,1)
        log_write(bp);
801012a4:	57                   	push   %edi
801012a5:	e8 76 1c 00 00       	call   80102f20 <log_write>
        brelse(bp);
801012aa:	89 3c 24             	mov    %edi,(%esp)
801012ad:	e8 3e ef ff ff       	call   801001f0 <brelse>
  bp = bread(dev, bno);
801012b2:	58                   	pop    %eax
801012b3:	5a                   	pop    %edx
801012b4:	56                   	push   %esi
801012b5:	ff 75 dc             	push   -0x24(%ebp)
801012b8:	e8 13 ee ff ff       	call   801000d0 <bread>
  memset(bp->data, 0, BSIZE);
801012bd:	83 c4 0c             	add    $0xc,%esp
  bp = bread(dev, bno);
801012c0:	89 c3                	mov    %eax,%ebx
  memset(bp->data, 0, BSIZE);
801012c2:	8d 40 5c             	lea    0x5c(%eax),%eax
801012c5:	68 00 02 00 00       	push   $0x200
801012ca:	6a 00                	push   $0x0
801012cc:	50                   	push   %eax
801012cd:	e8 ce 33 00 00       	call   801046a0 <memset>
  log_write(bp);
801012d2:	89 1c 24             	mov    %ebx,(%esp)
801012d5:	e8 46 1c 00 00       	call   80102f20 <log_write>
  brelse(bp);
801012da:	89 1c 24             	mov    %ebx,(%esp)
801012dd:	e8 0e ef ff ff       	call   801001f0 <brelse>
}
801012e2:	8d 65 f4             	lea    -0xc(%ebp),%esp
801012e5:	89 f0                	mov    %esi,%eax
801012e7:	5b                   	pop    %ebx
801012e8:	5e                   	pop    %esi
801012e9:	5f                   	pop    %edi
801012ea:	5d                   	pop    %ebp
801012eb:	c3                   	ret
801012ec:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi

801012f0 <iget>:
// Find the inode with number inum on device dev
// and return the in-memory copy. Does not lock
// the inode and does not read it from disk.
static struct inode*
iget(uint dev, uint inum)
{
801012f0:	55                   	push   %ebp
801012f1:	89 e5                	mov    %esp,%ebp
801012f3:	57                   	push   %edi
  struct inode *ip, *empty;

  acquire(&icache.lock);

  // Is the inode already cached?
  empty = 0;
801012f4:	31 ff                	xor    %edi,%edi
{
801012f6:	56                   	push   %esi
801012f7:	89 c6                	mov    %eax,%esi
801012f9:	53                   	push   %ebx
  for(ip = &icache.inode[0]; ip < &icache.inode[NINODE]; ip++){
801012fa:	bb 94 f9 10 80       	mov    $0x8010f994,%ebx
{
801012ff:	83 ec 28             	sub    $0x28,%esp
80101302:	89 55 e4             	mov    %edx,-0x1c(%ebp)
  acquire(&icache.lock);
80101305:	68 60 f9 10 80       	push   $0x8010f960
8010130a:	e8 91 32 00 00       	call   801045a0 <acquire>
  for(ip = &icache.inode[0]; ip < &icache.inode[NINODE]; ip++){
8010130f:	8b 55 e4             	mov    -0x1c(%ebp),%edx
  acquire(&icache.lock);
80101312:	83 c4 10             	add    $0x10,%esp
80101315:	eb 1b                	jmp    80101332 <iget+0x42>
80101317:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010131e:	00 
8010131f:	90                   	nop
    if(ip->ref > 0 && ip->dev == dev && ip->inum == inum){
80101320:	39 33                	cmp    %esi,(%ebx)
80101322:	74 6c                	je     80101390 <iget+0xa0>
  for(ip = &icache.inode[0]; ip < &icache.inode[NINODE]; ip++){
80101324:	81 c3 90 00 00 00    	add    $0x90,%ebx
8010132a:	81 fb b4 15 11 80    	cmp    $0x801115b4,%ebx
80101330:	74 26                	je     80101358 <iget+0x68>
    if(ip->ref > 0 && ip->dev == dev && ip->inum == inum){
80101332:	8b 43 08             	mov    0x8(%ebx),%eax
80101335:	85 c0                	test   %eax,%eax
80101337:	7f e7                	jg     80101320 <iget+0x30>
      ip->ref++;
      release(&icache.lock);
      return ip;
    }
    if(empty == 0 && ip->ref == 0)    // Remember empty slot.
80101339:	85 ff                	test   %edi,%edi
8010133b:	75 e7                	jne    80101324 <iget+0x34>
8010133d:	85 c0                	test   %eax,%eax
8010133f:	75 76                	jne    801013b7 <iget+0xc7>
      empty = ip;
80101341:	89 df                	mov    %ebx,%edi
  for(ip = &icache.inode[0]; ip < &icache.inode[NINODE]; ip++){
80101343:	81 c3 90 00 00 00    	add    $0x90,%ebx
80101349:	81 fb b4 15 11 80    	cmp    $0x801115b4,%ebx
8010134f:	75 e1                	jne    80101332 <iget+0x42>
80101351:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
  }

  // Recycle an inode cache entry.
  if(empty == 0)
80101358:	85 ff                	test   %edi,%edi
8010135a:	74 79                	je     801013d5 <iget+0xe5>
  ip = empty;
  ip->dev = dev;
  ip->inum = inum;
  ip->ref = 1;
  ip->valid = 0;
  release(&icache.lock);
8010135c:	83 ec 0c             	sub    $0xc,%esp
  ip->dev = dev;
8010135f:	89 37                	mov    %esi,(%edi)
  ip->inum = inum;
80101361:	89 57 04             	mov    %edx,0x4(%edi)
  ip->ref = 1;
80101364:	c7 47 08 01 00 00 00 	movl   $0x1,0x8(%edi)
  ip->valid = 0;
8010136b:	c7 47 4c 00 00 00 00 	movl   $0x0,0x4c(%edi)
  release(&icache.lock);
80101372:	68 60 f9 10 80       	push   $0x8010f960
80101377:	e8 c4 31 00 00       	call   80104540 <release>

  return ip;
8010137c:	83 c4 10             	add    $0x10,%esp
}
8010137f:	8d 65 f4             	lea    -0xc(%ebp),%esp
80101382:	89 f8                	mov    %edi,%eax
80101384:	5b                   	pop    %ebx
80101385:	5e                   	pop    %esi
80101386:	5f                   	pop    %edi
80101387:	5d                   	pop    %ebp
80101388:	c3                   	ret
80101389:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    if(ip->ref > 0 && ip->dev == dev && ip->inum == inum){
80101390:	39 53 04             	cmp    %edx,0x4(%ebx)
80101393:	75 8f                	jne    80101324 <iget+0x34>
      ip->ref++;
80101395:	83 c0 01             	add    $0x1,%eax
      release(&icache.lock);
80101398:	83 ec 0c             	sub    $0xc,%esp
      return ip;
8010139b:	89 df                	mov    %ebx,%edi
      ip->ref++;
8010139d:	89 43 08             	mov    %eax,0x8(%ebx)
      release(&icache.lock);
801013a0:	68 60 f9 10 80       	push   $0x8010f960
801013a5:	e8 96 31 00 00       	call   80104540 <release>
      return ip;
801013aa:	83 c4 10             	add    $0x10,%esp
}
801013ad:	8d 65 f4             	lea    -0xc(%ebp),%esp
801013b0:	89 f8                	mov    %edi,%eax
801013b2:	5b                   	pop    %ebx
801013b3:	5e                   	pop    %esi
801013b4:	5f                   	pop    %edi
801013b5:	5d                   	pop    %ebp
801013b6:	c3                   	ret
  for(ip = &icache.inode[0]; ip < &icache.inode[NINODE]; ip++){
801013b7:	81 c3 90 00 00 00    	add    $0x90,%ebx
801013bd:	81 fb b4 15 11 80    	cmp    $0x801115b4,%ebx
801013c3:	74 10                	je     801013d5 <iget+0xe5>
    if(ip->ref > 0 && ip->dev == dev && ip->inum == inum){
801013c5:	8b 43 08             	mov    0x8(%ebx),%eax
801013c8:	85 c0                	test   %eax,%eax
801013ca:	0f 8f 50 ff ff ff    	jg     80101320 <iget+0x30>
801013d0:	e9 68 ff ff ff       	jmp    8010133d <iget+0x4d>
    panic("iget: no inodes");
801013d5:	83 ec 0c             	sub    $0xc,%esp
801013d8:	68 24 72 10 80       	push   $0x80107224
801013dd:	e8 9e ef ff ff       	call   80100380 <panic>
801013e2:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801013e9:	00 
801013ea:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi

801013f0 <bfree>:
{
801013f0:	55                   	push   %ebp
801013f1:	89 c1                	mov    %eax,%ecx
  bp = bread(dev, BBLOCK(b, sb));
801013f3:	89 d0                	mov    %edx,%eax
801013f5:	c1 e8 0c             	shr    $0xc,%eax
{
801013f8:	89 e5                	mov    %esp,%ebp
801013fa:	56                   	push   %esi
801013fb:	53                   	push   %ebx
  bp = bread(dev, BBLOCK(b, sb));
801013fc:	03 05 cc 15 11 80    	add    0x801115cc,%eax
{
80101402:	89 d3                	mov    %edx,%ebx
  bp = bread(dev, BBLOCK(b, sb));
80101404:	83 ec 08             	sub    $0x8,%esp
80101407:	50                   	push   %eax
80101408:	51                   	push   %ecx
80101409:	e8 c2 ec ff ff       	call   801000d0 <bread>
  m = 1 << (bi % 8);
8010140e:	89 d9                	mov    %ebx,%ecx
  if((bp->data[bi/8] & m) == 0)
80101410:	c1 fb 03             	sar    $0x3,%ebx
80101413:	83 c4 10             	add    $0x10,%esp
  bp = bread(dev, BBLOCK(b, sb));
80101416:	89 c6                	mov    %eax,%esi
  m = 1 << (bi % 8);
80101418:	83 e1 07             	and    $0x7,%ecx
8010141b:	b8 01 00 00 00       	mov    $0x1,%eax
  if((bp->data[bi/8] & m) == 0)
80101420:	81 e3 ff 01 00 00    	and    $0x1ff,%ebx
  m = 1 << (bi % 8);
80101426:	d3 e0                	shl    %cl,%eax
  if((bp->data[bi/8] & m) == 0)
80101428:	0f b6 4c 1e 5c       	movzbl 0x5c(%esi,%ebx,1),%ecx
8010142d:	85 c1                	test   %eax,%ecx
8010142f:	74 23                	je     80101454 <bfree+0x64>
  bp->data[bi/8] &= ~m;
80101431:	f7 d0                	not    %eax
  log_write(bp);
80101433:	83 ec 0c             	sub    $0xc,%esp
  bp->data[bi/8] &= ~m;
80101436:	21 c8                	and    %ecx,%eax
80101438:	88 44 1e 5c          	mov    %al,0x5c(%esi,%ebx,1)
  log_write(bp);
8010143c:	56                   	push   %esi
8010143d:	e8 de 1a 00 00       	call   80102f20 <log_write>
  brelse(bp);
80101442:	89 34 24             	mov    %esi,(%esp)
80101445:	e8 a6 ed ff ff       	call   801001f0 <brelse>
}
8010144a:	83 c4 10             	add    $0x10,%esp
8010144d:	8d 65 f8             	lea    -0x8(%ebp),%esp
80101450:	5b                   	pop    %ebx
80101451:	5e                   	pop    %esi
80101452:	5d                   	pop    %ebp
80101453:	c3                   	ret
    panic("freeing free block");
80101454:	83 ec 0c             	sub    $0xc,%esp
80101457:	68 34 72 10 80       	push   $0x80107234
8010145c:	e8 1f ef ff ff       	call   80100380 <panic>
80101461:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80101468:	00 
80101469:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi

80101470 <bmap>:

// Return the disk block address of the nth block in inode ip.
// If there is no such block, bmap allocates one.
static uint
bmap(struct inode *ip, uint bn)
{
80101470:	55                   	push   %ebp
80101471:	89 e5                	mov    %esp,%ebp
80101473:	57                   	push   %edi
80101474:	56                   	push   %esi
80101475:	89 c6                	mov    %eax,%esi
80101477:	53                   	push   %ebx
80101478:	83 ec 1c             	sub    $0x1c,%esp
  uint addr, *a;
  struct buf *bp;

  if(bn < NDIRECT){
8010147b:	83 fa 0b             	cmp    $0xb,%edx
8010147e:	0f 86 8c 00 00 00    	jbe    80101510 <bmap+0xa0>
    if((addr = ip->addrs[bn]) == 0)
      ip->addrs[bn] = addr = balloc(ip->dev);
    return addr;
  }
  bn -= NDIRECT;
80101484:	8d 5a f4             	lea    -0xc(%edx),%ebx

  if(bn < NINDIRECT){
80101487:	83 fb 7f             	cmp    $0x7f,%ebx
8010148a:	0f 87 a2 00 00 00    	ja     80101532 <bmap+0xc2>
    // Load indirect block, allocating if necessary.
    if((addr = ip->addrs[NDIRECT]) == 0)
80101490:	8b 80 8c 00 00 00    	mov    0x8c(%eax),%eax
80101496:	85 c0                	test   %eax,%eax
80101498:	74 5e                	je     801014f8 <bmap+0x88>
      ip->addrs[NDIRECT] = addr = balloc(ip->dev);
    bp = bread(ip->dev, addr);
8010149a:	83 ec 08             	sub    $0x8,%esp
8010149d:	50                   	push   %eax
8010149e:	ff 36                	push   (%esi)
801014a0:	e8 2b ec ff ff       	call   801000d0 <bread>
    a = (uint*)bp->data;
    if((addr = a[bn]) == 0){
801014a5:	83 c4 10             	add    $0x10,%esp
801014a8:	8d 5c 98 5c          	lea    0x5c(%eax,%ebx,4),%ebx
    bp = bread(ip->dev, addr);
801014ac:	89 c2                	mov    %eax,%edx
    if((addr = a[bn]) == 0){
801014ae:	8b 3b                	mov    (%ebx),%edi
801014b0:	85 ff                	test   %edi,%edi
801014b2:	74 1c                	je     801014d0 <bmap+0x60>
      a[bn] = addr = balloc(ip->dev);
      log_write(bp);
    }
    brelse(bp);
801014b4:	83 ec 0c             	sub    $0xc,%esp
801014b7:	52                   	push   %edx
801014b8:	e8 33 ed ff ff       	call   801001f0 <brelse>
801014bd:	83 c4 10             	add    $0x10,%esp
    return addr;
  }

  panic("bmap: out of range");
}
801014c0:	8d 65 f4             	lea    -0xc(%ebp),%esp
801014c3:	89 f8                	mov    %edi,%eax
801014c5:	5b                   	pop    %ebx
801014c6:	5e                   	pop    %esi
801014c7:	5f                   	pop    %edi
801014c8:	5d                   	pop    %ebp
801014c9:	c3                   	ret
801014ca:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
801014d0:	89 45 e4             	mov    %eax,-0x1c(%ebp)
      a[bn] = addr = balloc(ip->dev);
801014d3:	8b 06                	mov    (%esi),%eax
801014d5:	e8 06 fd ff ff       	call   801011e0 <balloc>
      log_write(bp);
801014da:	8b 55 e4             	mov    -0x1c(%ebp),%edx
801014dd:	83 ec 0c             	sub    $0xc,%esp
      a[bn] = addr = balloc(ip->dev);
801014e0:	89 03                	mov    %eax,(%ebx)
801014e2:	89 c7                	mov    %eax,%edi
      log_write(bp);
801014e4:	52                   	push   %edx
801014e5:	e8 36 1a 00 00       	call   80102f20 <log_write>
801014ea:	8b 55 e4             	mov    -0x1c(%ebp),%edx
801014ed:	83 c4 10             	add    $0x10,%esp
801014f0:	eb c2                	jmp    801014b4 <bmap+0x44>
801014f2:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
      ip->addrs[NDIRECT] = addr = balloc(ip->dev);
801014f8:	8b 06                	mov    (%esi),%eax
801014fa:	e8 e1 fc ff ff       	call   801011e0 <balloc>
801014ff:	89 86 8c 00 00 00    	mov    %eax,0x8c(%esi)
80101505:	eb 93                	jmp    8010149a <bmap+0x2a>
80101507:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010150e:	00 
8010150f:	90                   	nop
    if((addr = ip->addrs[bn]) == 0)
80101510:	8d 5a 14             	lea    0x14(%edx),%ebx
80101513:	8b 7c 98 0c          	mov    0xc(%eax,%ebx,4),%edi
80101517:	85 ff                	test   %edi,%edi
80101519:	75 a5                	jne    801014c0 <bmap+0x50>
      ip->addrs[bn] = addr = balloc(ip->dev);
8010151b:	8b 00                	mov    (%eax),%eax
8010151d:	e8 be fc ff ff       	call   801011e0 <balloc>
80101522:	89 44 9e 0c          	mov    %eax,0xc(%esi,%ebx,4)
80101526:	89 c7                	mov    %eax,%edi
}
80101528:	8d 65 f4             	lea    -0xc(%ebp),%esp
8010152b:	5b                   	pop    %ebx
8010152c:	89 f8                	mov    %edi,%eax
8010152e:	5e                   	pop    %esi
8010152f:	5f                   	pop    %edi
80101530:	5d                   	pop    %ebp
80101531:	c3                   	ret
  panic("bmap: out of range");
80101532:	83 ec 0c             	sub    $0xc,%esp
80101535:	68 47 72 10 80       	push   $0x80107247
8010153a:	e8 41 ee ff ff       	call   80100380 <panic>
8010153f:	90                   	nop

80101540 <readsb>:
{
80101540:	55                   	push   %ebp
80101541:	89 e5                	mov    %esp,%ebp
80101543:	56                   	push   %esi
80101544:	53                   	push   %ebx
80101545:	8b 75 0c             	mov    0xc(%ebp),%esi
  bp = bread(dev, 1);
80101548:	83 ec 08             	sub    $0x8,%esp
8010154b:	6a 01                	push   $0x1
8010154d:	ff 75 08             	push   0x8(%ebp)
80101550:	e8 7b eb ff ff       	call   801000d0 <bread>
  memmove(sb, bp->data, sizeof(*sb));
80101555:	83 c4 0c             	add    $0xc,%esp
  bp = bread(dev, 1);
80101558:	89 c3                	mov    %eax,%ebx
  memmove(sb, bp->data, sizeof(*sb));
8010155a:	8d 40 5c             	lea    0x5c(%eax),%eax
8010155d:	6a 1c                	push   $0x1c
8010155f:	50                   	push   %eax
80101560:	56                   	push   %esi
80101561:	e8 ca 31 00 00       	call   80104730 <memmove>
  brelse(bp);
80101566:	83 c4 10             	add    $0x10,%esp
80101569:	89 5d 08             	mov    %ebx,0x8(%ebp)
}
8010156c:	8d 65 f8             	lea    -0x8(%ebp),%esp
8010156f:	5b                   	pop    %ebx
80101570:	5e                   	pop    %esi
80101571:	5d                   	pop    %ebp
  brelse(bp);
80101572:	e9 79 ec ff ff       	jmp    801001f0 <brelse>
80101577:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010157e:	00 
8010157f:	90                   	nop

80101580 <iinit>:
{
80101580:	55                   	push   %ebp
80101581:	89 e5                	mov    %esp,%ebp
80101583:	53                   	push   %ebx
80101584:	bb a0 f9 10 80       	mov    $0x8010f9a0,%ebx
80101589:	83 ec 0c             	sub    $0xc,%esp
  initlock(&icache.lock, "icache");
8010158c:	68 5a 72 10 80       	push   $0x8010725a
80101591:	68 60 f9 10 80       	push   $0x8010f960
80101596:	e8 15 2e 00 00       	call   801043b0 <initlock>
  for(i = 0; i < NINODE; i++) {
8010159b:	83 c4 10             	add    $0x10,%esp
8010159e:	66 90                	xchg   %ax,%ax
    initsleeplock(&icache.inode[i].lock, "inode");
801015a0:	83 ec 08             	sub    $0x8,%esp
801015a3:	68 61 72 10 80       	push   $0x80107261
801015a8:	53                   	push   %ebx
  for(i = 0; i < NINODE; i++) {
801015a9:	81 c3 90 00 00 00    	add    $0x90,%ebx
    initsleeplock(&icache.inode[i].lock, "inode");
801015af:	e8 cc 2c 00 00       	call   80104280 <initsleeplock>
  for(i = 0; i < NINODE; i++) {
801015b4:	83 c4 10             	add    $0x10,%esp
801015b7:	81 fb c0 15 11 80    	cmp    $0x801115c0,%ebx
801015bd:	75 e1                	jne    801015a0 <iinit+0x20>
  bp = bread(dev, 1);
801015bf:	83 ec 08             	sub    $0x8,%esp
801015c2:	6a 01                	push   $0x1
801015c4:	ff 75 08             	push   0x8(%ebp)
801015c7:	e8 04 eb ff ff       	call   801000d0 <bread>
  memmove(sb, bp->data, sizeof(*sb));
801015cc:	83 c4 0c             	add    $0xc,%esp
  bp = bread(dev, 1);
801015cf:	89 c3                	mov    %eax,%ebx
  memmove(sb, bp->data, sizeof(*sb));
801015d1:	8d 40 5c             	lea    0x5c(%eax),%eax
801015d4:	6a 1c                	push   $0x1c
801015d6:	50                   	push   %eax
801015d7:	68 b4 15 11 80       	push   $0x801115b4
801015dc:	e8 4f 31 00 00       	call   80104730 <memmove>
  brelse(bp);
801015e1:	89 1c 24             	mov    %ebx,(%esp)
801015e4:	e8 07 ec ff ff       	call   801001f0 <brelse>
  cprintf("sb: size %d nblocks %d ninodes %d nlog %d logstart %d\
801015e9:	ff 35 cc 15 11 80    	push   0x801115cc
801015ef:	ff 35 c8 15 11 80    	push   0x801115c8
801015f5:	ff 35 c4 15 11 80    	push   0x801115c4
801015fb:	ff 35 c0 15 11 80    	push   0x801115c0
80101601:	ff 35 bc 15 11 80    	push   0x801115bc
80101607:	ff 35 b8 15 11 80    	push   0x801115b8
8010160d:	ff 35 b4 15 11 80    	push   0x801115b4
80101613:	68 74 76 10 80       	push   $0x80107674
80101618:	e8 93 f0 ff ff       	call   801006b0 <cprintf>
}
8010161d:	8b 5d fc             	mov    -0x4(%ebp),%ebx
80101620:	83 c4 30             	add    $0x30,%esp
80101623:	c9                   	leave
80101624:	c3                   	ret
80101625:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010162c:	00 
8010162d:	8d 76 00             	lea    0x0(%esi),%esi

80101630 <ialloc>:
{
80101630:	55                   	push   %ebp
80101631:	89 e5                	mov    %esp,%ebp
80101633:	57                   	push   %edi
80101634:	56                   	push   %esi
80101635:	53                   	push   %ebx
80101636:	83 ec 1c             	sub    $0x1c,%esp
80101639:	8b 45 0c             	mov    0xc(%ebp),%eax
  for(inum = 1; inum < sb.ninodes; inum++){
8010163c:	83 3d bc 15 11 80 01 	cmpl   $0x1,0x801115bc
{
80101643:	8b 75 08             	mov    0x8(%ebp),%esi
80101646:	89 45 e4             	mov    %eax,-0x1c(%ebp)
  for(inum = 1; inum < sb.ninodes; inum++){
80101649:	0f 86 91 00 00 00    	jbe    801016e0 <ialloc+0xb0>
8010164f:	bf 01 00 00 00       	mov    $0x1,%edi
80101654:	eb 21                	jmp    80101677 <ialloc+0x47>
80101656:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010165d:	00 
8010165e:	66 90                	xchg   %ax,%ax
    brelse(bp);
80101660:	83 ec 0c             	sub    $0xc,%esp
  for(inum = 1; inum < sb.ninodes; inum++){
80101663:	83 c7 01             	add    $0x1,%edi
    brelse(bp);
80101666:	53                   	push   %ebx
80101667:	e8 84 eb ff ff       	call   801001f0 <brelse>
  for(inum = 1; inum < sb.ninodes; inum++){
8010166c:	83 c4 10             	add    $0x10,%esp
8010166f:	3b 3d bc 15 11 80    	cmp    0x801115bc,%edi
80101675:	73 69                	jae    801016e0 <ialloc+0xb0>
    bp = bread(dev, IBLOCK(inum, sb));
80101677:	89 f8                	mov    %edi,%eax
80101679:	83 ec 08             	sub    $0x8,%esp
8010167c:	c1 e8 03             	shr    $0x3,%eax
8010167f:	03 05 c8 15 11 80    	add    0x801115c8,%eax
80101685:	50                   	push   %eax
80101686:	56                   	push   %esi
80101687:	e8 44 ea ff ff       	call   801000d0 <bread>
    if(dip->type == 0){  // a free inode
8010168c:	83 c4 10             	add    $0x10,%esp
    bp = bread(dev, IBLOCK(inum, sb));
8010168f:	89 c3                	mov    %eax,%ebx
    dip = (struct dinode*)bp->data + inum%IPB;
80101691:	89 f8                	mov    %edi,%eax
80101693:	83 e0 07             	and    $0x7,%eax
80101696:	c1 e0 06             	shl    $0x6,%eax
80101699:	8d 4c 03 5c          	lea    0x5c(%ebx,%eax,1),%ecx
    if(dip->type == 0){  // a free inode
8010169d:	66 83 39 00          	cmpw   $0x0,(%ecx)
801016a1:	75 bd                	jne    80101660 <ialloc+0x30>
      memset(dip, 0, sizeof(*dip));
801016a3:	83 ec 04             	sub    $0x4,%esp
801016a6:	6a 40                	push   $0x40
801016a8:	6a 00                	push   $0x0
801016aa:	51                   	push   %ecx
801016ab:	89 4d e0             	mov    %ecx,-0x20(%ebp)
801016ae:	e8 ed 2f 00 00       	call   801046a0 <memset>
      dip->type = type;
801016b3:	0f b7 45 e4          	movzwl -0x1c(%ebp),%eax
801016b7:	8b 4d e0             	mov    -0x20(%ebp),%ecx
801016ba:	66 89 01             	mov    %ax,(%ecx)
      log_write(bp);   // mark it allocated on the disk
801016bd:	89 1c 24             	mov    %ebx,(%esp)
801016c0:	e8 5b 18 00 00       	call   80102f20 <log_write>
      brelse(bp);
801016c5:	89 1c 24             	mov    %ebx,(%esp)
801016c8:	e8 23 eb ff ff       	call   801001f0 <brelse>
      return iget(dev, inum);
801016cd:	83 c4 10             	add    $0x10,%esp
}
801016d0:	8d 65 f4             	lea    -0xc(%ebp),%esp
      return iget(dev, inum);
801016d3:	89 fa                	mov    %edi,%edx
}
801016d5:	5b                   	pop    %ebx
      return iget(dev, inum);
801016d6:	89 f0                	mov    %esi,%eax
}
801016d8:	5e                   	pop    %esi
801016d9:	5f                   	pop    %edi
801016da:	5d                   	pop    %ebp
      return iget(dev, inum);
801016db:	e9 10 fc ff ff       	jmp    801012f0 <iget>
  panic("ialloc: no inodes");
801016e0:	83 ec 0c             	sub    $0xc,%esp
801016e3:	68 67 72 10 80       	push   $0x80107267
801016e8:	e8 93 ec ff ff       	call   80100380 <panic>
801016ed:	8d 76 00             	lea    0x0(%esi),%esi

801016f0 <iupdate>:
{
801016f0:	55                   	push   %ebp
801016f1:	89 e5                	mov    %esp,%ebp
801016f3:	56                   	push   %esi
801016f4:	53                   	push   %ebx
801016f5:	8b 5d 08             	mov    0x8(%ebp),%ebx
  bp = bread(ip->dev, IBLOCK(ip->inum, sb));
801016f8:	8b 43 04             	mov    0x4(%ebx),%eax
  memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
801016fb:	83 c3 5c             	add    $0x5c,%ebx
  bp = bread(ip->dev, IBLOCK(ip->inum, sb));
801016fe:	83 ec 08             	sub    $0x8,%esp
80101701:	c1 e8 03             	shr    $0x3,%eax
80101704:	03 05 c8 15 11 80    	add    0x801115c8,%eax
8010170a:	50                   	push   %eax
8010170b:	ff 73 a4             	push   -0x5c(%ebx)
8010170e:	e8 bd e9 ff ff       	call   801000d0 <bread>
  dip->type = ip->type;
80101713:	0f b7 53 f4          	movzwl -0xc(%ebx),%edx
  memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
80101717:	83 c4 0c             	add    $0xc,%esp
  bp = bread(ip->dev, IBLOCK(ip->inum, sb));
8010171a:	89 c6                	mov    %eax,%esi
  dip = (struct dinode*)bp->data + ip->inum%IPB;
8010171c:	8b 43 a8             	mov    -0x58(%ebx),%eax
8010171f:	83 e0 07             	and    $0x7,%eax
80101722:	c1 e0 06             	shl    $0x6,%eax
80101725:	8d 44 06 5c          	lea    0x5c(%esi,%eax,1),%eax
  dip->type = ip->type;
80101729:	66 89 10             	mov    %dx,(%eax)
  dip->major = ip->major;
8010172c:	0f b7 53 f6          	movzwl -0xa(%ebx),%edx
  memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
80101730:	83 c0 0c             	add    $0xc,%eax
  dip->major = ip->major;
80101733:	66 89 50 f6          	mov    %dx,-0xa(%eax)
  dip->minor = ip->minor;
80101737:	0f b7 53 f8          	movzwl -0x8(%ebx),%edx
8010173b:	66 89 50 f8          	mov    %dx,-0x8(%eax)
  dip->nlink = ip->nlink;
8010173f:	0f b7 53 fa          	movzwl -0x6(%ebx),%edx
80101743:	66 89 50 fa          	mov    %dx,-0x6(%eax)
  dip->size = ip->size;
80101747:	8b 53 fc             	mov    -0x4(%ebx),%edx
8010174a:	89 50 fc             	mov    %edx,-0x4(%eax)
  memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
8010174d:	6a 34                	push   $0x34
8010174f:	53                   	push   %ebx
80101750:	50                   	push   %eax
80101751:	e8 da 2f 00 00       	call   80104730 <memmove>
  log_write(bp);
80101756:	89 34 24             	mov    %esi,(%esp)
80101759:	e8 c2 17 00 00       	call   80102f20 <log_write>
  brelse(bp);
8010175e:	83 c4 10             	add    $0x10,%esp
80101761:	89 75 08             	mov    %esi,0x8(%ebp)
}
80101764:	8d 65 f8             	lea    -0x8(%ebp),%esp
80101767:	5b                   	pop    %ebx
80101768:	5e                   	pop    %esi
80101769:	5d                   	pop    %ebp
  brelse(bp);
8010176a:	e9 81 ea ff ff       	jmp    801001f0 <brelse>
8010176f:	90                   	nop

80101770 <idup>:
{
80101770:	55                   	push   %ebp
80101771:	89 e5                	mov    %esp,%ebp
80101773:	53                   	push   %ebx
80101774:	83 ec 10             	sub    $0x10,%esp
80101777:	8b 5d 08             	mov    0x8(%ebp),%ebx
  acquire(&icache.lock);
8010177a:	68 60 f9 10 80       	push   $0x8010f960
8010177f:	e8 1c 2e 00 00       	call   801045a0 <acquire>
  ip->ref++;
80101784:	83 43 08 01          	addl   $0x1,0x8(%ebx)
  release(&icache.lock);
80101788:	c7 04 24 60 f9 10 80 	movl   $0x8010f960,(%esp)
8010178f:	e8 ac 2d 00 00       	call   80104540 <release>
}
80101794:	89 d8                	mov    %ebx,%eax
80101796:	8b 5d fc             	mov    -0x4(%ebp),%ebx
80101799:	c9                   	leave
8010179a:	c3                   	ret
8010179b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi

801017a0 <ilock>:
{
801017a0:	55                   	push   %ebp
801017a1:	89 e5                	mov    %esp,%ebp
801017a3:	56                   	push   %esi
801017a4:	53                   	push   %ebx
801017a5:	8b 5d 08             	mov    0x8(%ebp),%ebx
  if(ip == 0 || ip->ref < 1)
801017a8:	85 db                	test   %ebx,%ebx
801017aa:	0f 84 b7 00 00 00    	je     80101867 <ilock+0xc7>
801017b0:	8b 53 08             	mov    0x8(%ebx),%edx
801017b3:	85 d2                	test   %edx,%edx
801017b5:	0f 8e ac 00 00 00    	jle    80101867 <ilock+0xc7>
  acquiresleep(&ip->lock);
801017bb:	83 ec 0c             	sub    $0xc,%esp
801017be:	8d 43 0c             	lea    0xc(%ebx),%eax
801017c1:	50                   	push   %eax
801017c2:	e8 f9 2a 00 00       	call   801042c0 <acquiresleep>
  if(ip->valid == 0){
801017c7:	8b 43 4c             	mov    0x4c(%ebx),%eax
801017ca:	83 c4 10             	add    $0x10,%esp
801017cd:	85 c0                	test   %eax,%eax
801017cf:	74 0f                	je     801017e0 <ilock+0x40>
}
801017d1:	8d 65 f8             	lea    -0x8(%ebp),%esp
801017d4:	5b                   	pop    %ebx
801017d5:	5e                   	pop    %esi
801017d6:	5d                   	pop    %ebp
801017d7:	c3                   	ret
801017d8:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801017df:	00 
    bp = bread(ip->dev, IBLOCK(ip->inum, sb));
801017e0:	8b 43 04             	mov    0x4(%ebx),%eax
801017e3:	83 ec 08             	sub    $0x8,%esp
801017e6:	c1 e8 03             	shr    $0x3,%eax
801017e9:	03 05 c8 15 11 80    	add    0x801115c8,%eax
801017ef:	50                   	push   %eax
801017f0:	ff 33                	push   (%ebx)
801017f2:	e8 d9 e8 ff ff       	call   801000d0 <bread>
    memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
801017f7:	83 c4 0c             	add    $0xc,%esp
    bp = bread(ip->dev, IBLOCK(ip->inum, sb));
801017fa:	89 c6                	mov    %eax,%esi
    dip = (struct dinode*)bp->data + ip->inum%IPB;
801017fc:	8b 43 04             	mov    0x4(%ebx),%eax
801017ff:	83 e0 07             	and    $0x7,%eax
80101802:	c1 e0 06             	shl    $0x6,%eax
80101805:	8d 44 06 5c          	lea    0x5c(%esi,%eax,1),%eax
    ip->type = dip->type;
80101809:	0f b7 10             	movzwl (%eax),%edx
    memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
8010180c:	83 c0 0c             	add    $0xc,%eax
    ip->type = dip->type;
8010180f:	66 89 53 50          	mov    %dx,0x50(%ebx)
    ip->major = dip->major;
80101813:	0f b7 50 f6          	movzwl -0xa(%eax),%edx
80101817:	66 89 53 52          	mov    %dx,0x52(%ebx)
    ip->minor = dip->minor;
8010181b:	0f b7 50 f8          	movzwl -0x8(%eax),%edx
8010181f:	66 89 53 54          	mov    %dx,0x54(%ebx)
    ip->nlink = dip->nlink;
80101823:	0f b7 50 fa          	movzwl -0x6(%eax),%edx
80101827:	66 89 53 56          	mov    %dx,0x56(%ebx)
    ip->size = dip->size;
8010182b:	8b 50 fc             	mov    -0x4(%eax),%edx
8010182e:	89 53 58             	mov    %edx,0x58(%ebx)
    memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
80101831:	6a 34                	push   $0x34
80101833:	50                   	push   %eax
80101834:	8d 43 5c             	lea    0x5c(%ebx),%eax
80101837:	50                   	push   %eax
80101838:	e8 f3 2e 00 00       	call   80104730 <memmove>
    brelse(bp);
8010183d:	89 34 24             	mov    %esi,(%esp)
80101840:	e8 ab e9 ff ff       	call   801001f0 <brelse>
    if(ip->type == 0)
80101845:	83 c4 10             	add    $0x10,%esp
80101848:	66 83 7b 50 00       	cmpw   $0x0,0x50(%ebx)
    ip->valid = 1;
8010184d:	c7 43 4c 01 00 00 00 	movl   $0x1,0x4c(%ebx)
    if(ip->type == 0)
80101854:	0f 85 77 ff ff ff    	jne    801017d1 <ilock+0x31>
      panic("ilock: no type");
8010185a:	83 ec 0c             	sub    $0xc,%esp
8010185d:	68 7f 72 10 80       	push   $0x8010727f
80101862:	e8 19 eb ff ff       	call   80100380 <panic>
    panic("ilock");
80101867:	83 ec 0c             	sub    $0xc,%esp
8010186a:	68 79 72 10 80       	push   $0x80107279
8010186f:	e8 0c eb ff ff       	call   80100380 <panic>
80101874:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010187b:	00 
8010187c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi

80101880 <iunlock>:
{
80101880:	55                   	push   %ebp
80101881:	89 e5                	mov    %esp,%ebp
80101883:	56                   	push   %esi
80101884:	53                   	push   %ebx
80101885:	8b 5d 08             	mov    0x8(%ebp),%ebx
  if(ip == 0 || !holdingsleep(&ip->lock) || ip->ref < 1)
80101888:	85 db                	test   %ebx,%ebx
8010188a:	74 28                	je     801018b4 <iunlock+0x34>
8010188c:	83 ec 0c             	sub    $0xc,%esp
8010188f:	8d 73 0c             	lea    0xc(%ebx),%esi
80101892:	56                   	push   %esi
80101893:	e8 c8 2a 00 00       	call   80104360 <holdingsleep>
80101898:	83 c4 10             	add    $0x10,%esp
8010189b:	85 c0                	test   %eax,%eax
8010189d:	74 15                	je     801018b4 <iunlock+0x34>
8010189f:	8b 43 08             	mov    0x8(%ebx),%eax
801018a2:	85 c0                	test   %eax,%eax
801018a4:	7e 0e                	jle    801018b4 <iunlock+0x34>
  releasesleep(&ip->lock);
801018a6:	89 75 08             	mov    %esi,0x8(%ebp)
}
801018a9:	8d 65 f8             	lea    -0x8(%ebp),%esp
801018ac:	5b                   	pop    %ebx
801018ad:	5e                   	pop    %esi
801018ae:	5d                   	pop    %ebp
  releasesleep(&ip->lock);
801018af:	e9 6c 2a 00 00       	jmp    80104320 <releasesleep>
    panic("iunlock");
801018b4:	83 ec 0c             	sub    $0xc,%esp
801018b7:	68 8e 72 10 80       	push   $0x8010728e
801018bc:	e8 bf ea ff ff       	call   80100380 <panic>
801018c1:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801018c8:	00 
801018c9:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi

801018d0 <iput>:
{
801018d0:	55                   	push   %ebp
801018d1:	89 e5                	mov    %esp,%ebp
801018d3:	57                   	push   %edi
801018d4:	56                   	push   %esi
801018d5:	53                   	push   %ebx
801018d6:	83 ec 28             	sub    $0x28,%esp
801018d9:	8b 5d 08             	mov    0x8(%ebp),%ebx
  acquiresleep(&ip->lock);
801018dc:	8d 7b 0c             	lea    0xc(%ebx),%edi
801018df:	57                   	push   %edi
801018e0:	e8 db 29 00 00       	call   801042c0 <acquiresleep>
  if(ip->valid && ip->nlink == 0){
801018e5:	8b 53 4c             	mov    0x4c(%ebx),%edx
801018e8:	83 c4 10             	add    $0x10,%esp
801018eb:	85 d2                	test   %edx,%edx
801018ed:	74 07                	je     801018f6 <iput+0x26>
801018ef:	66 83 7b 56 00       	cmpw   $0x0,0x56(%ebx)
801018f4:	74 32                	je     80101928 <iput+0x58>
  releasesleep(&ip->lock);
801018f6:	83 ec 0c             	sub    $0xc,%esp
801018f9:	57                   	push   %edi
801018fa:	e8 21 2a 00 00       	call   80104320 <releasesleep>
  acquire(&icache.lock);
801018ff:	c7 04 24 60 f9 10 80 	movl   $0x8010f960,(%esp)
80101906:	e8 95 2c 00 00       	call   801045a0 <acquire>
  ip->ref--;
8010190b:	83 6b 08 01          	subl   $0x1,0x8(%ebx)
  release(&icache.lock);
8010190f:	83 c4 10             	add    $0x10,%esp
80101912:	c7 45 08 60 f9 10 80 	movl   $0x8010f960,0x8(%ebp)
}
80101919:	8d 65 f4             	lea    -0xc(%ebp),%esp
8010191c:	5b                   	pop    %ebx
8010191d:	5e                   	pop    %esi
8010191e:	5f                   	pop    %edi
8010191f:	5d                   	pop    %ebp
  release(&icache.lock);
80101920:	e9 1b 2c 00 00       	jmp    80104540 <release>
80101925:	8d 76 00             	lea    0x0(%esi),%esi
    acquire(&icache.lock);
80101928:	83 ec 0c             	sub    $0xc,%esp
8010192b:	68 60 f9 10 80       	push   $0x8010f960
80101930:	e8 6b 2c 00 00       	call   801045a0 <acquire>
    int r = ip->ref;
80101935:	8b 73 08             	mov    0x8(%ebx),%esi
    release(&icache.lock);
80101938:	c7 04 24 60 f9 10 80 	movl   $0x8010f960,(%esp)
8010193f:	e8 fc 2b 00 00       	call   80104540 <release>
    if(r == 1){
80101944:	83 c4 10             	add    $0x10,%esp
80101947:	83 fe 01             	cmp    $0x1,%esi
8010194a:	75 aa                	jne    801018f6 <iput+0x26>
8010194c:	8d 8b 8c 00 00 00    	lea    0x8c(%ebx),%ecx
80101952:	89 7d e4             	mov    %edi,-0x1c(%ebp)
80101955:	8d 73 5c             	lea    0x5c(%ebx),%esi
80101958:	89 df                	mov    %ebx,%edi
8010195a:	89 cb                	mov    %ecx,%ebx
8010195c:	eb 09                	jmp    80101967 <iput+0x97>
8010195e:	66 90                	xchg   %ax,%ax
{
  int i, j;
  struct buf *bp;
  uint *a;

  for(i = 0; i < NDIRECT; i++){
80101960:	83 c6 04             	add    $0x4,%esi
80101963:	39 de                	cmp    %ebx,%esi
80101965:	74 19                	je     80101980 <iput+0xb0>
    if(ip->addrs[i]){
80101967:	8b 16                	mov    (%esi),%edx
80101969:	85 d2                	test   %edx,%edx
8010196b:	74 f3                	je     80101960 <iput+0x90>
      bfree(ip->dev, ip->addrs[i]);
8010196d:	8b 07                	mov    (%edi),%eax
8010196f:	e8 7c fa ff ff       	call   801013f0 <bfree>
      ip->addrs[i] = 0;
80101974:	c7 06 00 00 00 00    	movl   $0x0,(%esi)
8010197a:	eb e4                	jmp    80101960 <iput+0x90>
8010197c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    }
  }

  if(ip->addrs[NDIRECT]){
80101980:	89 fb                	mov    %edi,%ebx
80101982:	8b 7d e4             	mov    -0x1c(%ebp),%edi
80101985:	8b 83 8c 00 00 00    	mov    0x8c(%ebx),%eax
8010198b:	85 c0                	test   %eax,%eax
8010198d:	75 2d                	jne    801019bc <iput+0xec>
    bfree(ip->dev, ip->addrs[NDIRECT]);
    ip->addrs[NDIRECT] = 0;
  }

  ip->size = 0;
  iupdate(ip);
8010198f:	83 ec 0c             	sub    $0xc,%esp
  ip->size = 0;
80101992:	c7 43 58 00 00 00 00 	movl   $0x0,0x58(%ebx)
  iupdate(ip);
80101999:	53                   	push   %ebx
8010199a:	e8 51 fd ff ff       	call   801016f0 <iupdate>
      ip->type = 0;
8010199f:	31 c0                	xor    %eax,%eax
801019a1:	66 89 43 50          	mov    %ax,0x50(%ebx)
      iupdate(ip);
801019a5:	89 1c 24             	mov    %ebx,(%esp)
801019a8:	e8 43 fd ff ff       	call   801016f0 <iupdate>
      ip->valid = 0;
801019ad:	c7 43 4c 00 00 00 00 	movl   $0x0,0x4c(%ebx)
801019b4:	83 c4 10             	add    $0x10,%esp
801019b7:	e9 3a ff ff ff       	jmp    801018f6 <iput+0x26>
    bp = bread(ip->dev, ip->addrs[NDIRECT]);
801019bc:	83 ec 08             	sub    $0x8,%esp
801019bf:	50                   	push   %eax
801019c0:	ff 33                	push   (%ebx)
801019c2:	e8 09 e7 ff ff       	call   801000d0 <bread>
    for(j = 0; j < NINDIRECT; j++){
801019c7:	83 c4 10             	add    $0x10,%esp
801019ca:	89 7d e4             	mov    %edi,-0x1c(%ebp)
801019cd:	8d 88 5c 02 00 00    	lea    0x25c(%eax),%ecx
801019d3:	89 45 e0             	mov    %eax,-0x20(%ebp)
801019d6:	8d 70 5c             	lea    0x5c(%eax),%esi
801019d9:	89 cf                	mov    %ecx,%edi
801019db:	eb 0a                	jmp    801019e7 <iput+0x117>
801019dd:	8d 76 00             	lea    0x0(%esi),%esi
801019e0:	83 c6 04             	add    $0x4,%esi
801019e3:	39 fe                	cmp    %edi,%esi
801019e5:	74 0f                	je     801019f6 <iput+0x126>
      if(a[j])
801019e7:	8b 16                	mov    (%esi),%edx
801019e9:	85 d2                	test   %edx,%edx
801019eb:	74 f3                	je     801019e0 <iput+0x110>
        bfree(ip->dev, a[j]);
801019ed:	8b 03                	mov    (%ebx),%eax
801019ef:	e8 fc f9 ff ff       	call   801013f0 <bfree>
801019f4:	eb ea                	jmp    801019e0 <iput+0x110>
    brelse(bp);
801019f6:	8b 45 e0             	mov    -0x20(%ebp),%eax
801019f9:	83 ec 0c             	sub    $0xc,%esp
801019fc:	8b 7d e4             	mov    -0x1c(%ebp),%edi
801019ff:	50                   	push   %eax
80101a00:	e8 eb e7 ff ff       	call   801001f0 <brelse>
    bfree(ip->dev, ip->addrs[NDIRECT]);
80101a05:	8b 93 8c 00 00 00    	mov    0x8c(%ebx),%edx
80101a0b:	8b 03                	mov    (%ebx),%eax
80101a0d:	e8 de f9 ff ff       	call   801013f0 <bfree>
    ip->addrs[NDIRECT] = 0;
80101a12:	83 c4 10             	add    $0x10,%esp
80101a15:	c7 83 8c 00 00 00 00 	movl   $0x0,0x8c(%ebx)
80101a1c:	00 00 00 
80101a1f:	e9 6b ff ff ff       	jmp    8010198f <iput+0xbf>
80101a24:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80101a2b:	00 
80101a2c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi

80101a30 <iunlockput>:
{
80101a30:	55                   	push   %ebp
80101a31:	89 e5                	mov    %esp,%ebp
80101a33:	56                   	push   %esi
80101a34:	53                   	push   %ebx
80101a35:	8b 5d 08             	mov    0x8(%ebp),%ebx
  if(ip == 0 || !holdingsleep(&ip->lock) || ip->ref < 1)
80101a38:	85 db                	test   %ebx,%ebx
80101a3a:	74 34                	je     80101a70 <iunlockput+0x40>
80101a3c:	83 ec 0c             	sub    $0xc,%esp
80101a3f:	8d 73 0c             	lea    0xc(%ebx),%esi
80101a42:	56                   	push   %esi
80101a43:	e8 18 29 00 00       	call   80104360 <holdingsleep>
80101a48:	83 c4 10             	add    $0x10,%esp
80101a4b:	85 c0                	test   %eax,%eax
80101a4d:	74 21                	je     80101a70 <iunlockput+0x40>
80101a4f:	8b 43 08             	mov    0x8(%ebx),%eax
80101a52:	85 c0                	test   %eax,%eax
80101a54:	7e 1a                	jle    80101a70 <iunlockput+0x40>
  releasesleep(&ip->lock);
80101a56:	83 ec 0c             	sub    $0xc,%esp
80101a59:	56                   	push   %esi
80101a5a:	e8 c1 28 00 00       	call   80104320 <releasesleep>
  iput(ip);
80101a5f:	83 c4 10             	add    $0x10,%esp
80101a62:	89 5d 08             	mov    %ebx,0x8(%ebp)
}
80101a65:	8d 65 f8             	lea    -0x8(%ebp),%esp
80101a68:	5b                   	pop    %ebx
80101a69:	5e                   	pop    %esi
80101a6a:	5d                   	pop    %ebp
  iput(ip);
80101a6b:	e9 60 fe ff ff       	jmp    801018d0 <iput>
    panic("iunlock");
80101a70:	83 ec 0c             	sub    $0xc,%esp
80101a73:	68 8e 72 10 80       	push   $0x8010728e
80101a78:	e8 03 e9 ff ff       	call   80100380 <panic>
80101a7d:	8d 76 00             	lea    0x0(%esi),%esi

80101a80 <stati>:

// Copy stat information from inode.
// Caller must hold ip->lock.
void
stati(struct inode *ip, struct stat *st)
{
80101a80:	55                   	push   %ebp
80101a81:	89 e5                	mov    %esp,%ebp
80101a83:	8b 55 08             	mov    0x8(%ebp),%edx
80101a86:	8b 45 0c             	mov    0xc(%ebp),%eax
  st->dev = ip->dev;
80101a89:	8b 0a                	mov    (%edx),%ecx
80101a8b:	89 48 04             	mov    %ecx,0x4(%eax)
  st->ino = ip->inum;
80101a8e:	8b 4a 04             	mov    0x4(%edx),%ecx
80101a91:	89 48 08             	mov    %ecx,0x8(%eax)
  st->type = ip->type;
80101a94:	0f b7 4a 50          	movzwl 0x50(%edx),%ecx
80101a98:	66 89 08             	mov    %cx,(%eax)
  st->nlink = ip->nlink;
80101a9b:	0f b7 4a 56          	movzwl 0x56(%edx),%ecx
80101a9f:	66 89 48 0c          	mov    %cx,0xc(%eax)
  st->size = ip->size;
80101aa3:	8b 52 58             	mov    0x58(%edx),%edx
80101aa6:	89 50 10             	mov    %edx,0x10(%eax)
}
80101aa9:	5d                   	pop    %ebp
80101aaa:	c3                   	ret
80101aab:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi

80101ab0 <readi>:
//PAGEBREAK!
// Read data from inode.
// Caller must hold ip->lock.
int
readi(struct inode *ip, char *dst, uint off, uint n)
{
80101ab0:	55                   	push   %ebp
80101ab1:	89 e5                	mov    %esp,%ebp
80101ab3:	57                   	push   %edi
80101ab4:	56                   	push   %esi
80101ab5:	53                   	push   %ebx
80101ab6:	83 ec 1c             	sub    $0x1c,%esp
80101ab9:	8b 75 08             	mov    0x8(%ebp),%esi
80101abc:	8b 45 0c             	mov    0xc(%ebp),%eax
80101abf:	8b 7d 10             	mov    0x10(%ebp),%edi
  uint tot, m;
  struct buf *bp;

  if(ip->type == T_DEV){
80101ac2:	66 83 7e 50 03       	cmpw   $0x3,0x50(%esi)
{
80101ac7:	89 45 e0             	mov    %eax,-0x20(%ebp)
80101aca:	89 75 d8             	mov    %esi,-0x28(%ebp)
80101acd:	8b 45 14             	mov    0x14(%ebp),%eax
  if(ip->type == T_DEV){
80101ad0:	0f 84 aa 00 00 00    	je     80101b80 <readi+0xd0>
    if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].read)
      return -1;
    return devsw[ip->major].read(ip, dst, n);
  }

  if(off > ip->size || off + n < off)
80101ad6:	8b 75 d8             	mov    -0x28(%ebp),%esi
80101ad9:	8b 56 58             	mov    0x58(%esi),%edx
80101adc:	39 fa                	cmp    %edi,%edx
80101ade:	0f 82 bd 00 00 00    	jb     80101ba1 <readi+0xf1>
80101ae4:	89 f9                	mov    %edi,%ecx
80101ae6:	31 db                	xor    %ebx,%ebx
80101ae8:	01 c1                	add    %eax,%ecx
80101aea:	0f 92 c3             	setb   %bl
80101aed:	89 5d e4             	mov    %ebx,-0x1c(%ebp)
80101af0:	0f 82 ab 00 00 00    	jb     80101ba1 <readi+0xf1>
    return -1;
  if(off + n > ip->size)
    n = ip->size - off;
80101af6:	89 d3                	mov    %edx,%ebx
80101af8:	29 fb                	sub    %edi,%ebx
80101afa:	39 ca                	cmp    %ecx,%edx
80101afc:	0f 42 c3             	cmovb  %ebx,%eax

  for(tot=0; tot<n; tot+=m, off+=m, dst+=m){
80101aff:	85 c0                	test   %eax,%eax
80101b01:	74 73                	je     80101b76 <readi+0xc6>
    bp = bread(ip->dev, bmap(ip, off/BSIZE));
    m = min(n - tot, BSIZE - off%BSIZE);
80101b03:	8b 75 e4             	mov    -0x1c(%ebp),%esi
80101b06:	89 45 e4             	mov    %eax,-0x1c(%ebp)
80101b09:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    bp = bread(ip->dev, bmap(ip, off/BSIZE));
80101b10:	8b 5d d8             	mov    -0x28(%ebp),%ebx
80101b13:	89 fa                	mov    %edi,%edx
80101b15:	c1 ea 09             	shr    $0x9,%edx
80101b18:	89 d8                	mov    %ebx,%eax
80101b1a:	e8 51 f9 ff ff       	call   80101470 <bmap>
80101b1f:	83 ec 08             	sub    $0x8,%esp
80101b22:	50                   	push   %eax
80101b23:	ff 33                	push   (%ebx)
80101b25:	e8 a6 e5 ff ff       	call   801000d0 <bread>
    m = min(n - tot, BSIZE - off%BSIZE);
80101b2a:	8b 5d e4             	mov    -0x1c(%ebp),%ebx
80101b2d:	b9 00 02 00 00       	mov    $0x200,%ecx
    bp = bread(ip->dev, bmap(ip, off/BSIZE));
80101b32:	89 c2                	mov    %eax,%edx
    m = min(n - tot, BSIZE - off%BSIZE);
80101b34:	89 f8                	mov    %edi,%eax
80101b36:	25 ff 01 00 00       	and    $0x1ff,%eax
80101b3b:	29 f3                	sub    %esi,%ebx
80101b3d:	29 c1                	sub    %eax,%ecx
    memmove(dst, bp->data + off%BSIZE, m);
80101b3f:	8d 44 02 5c          	lea    0x5c(%edx,%eax,1),%eax
    m = min(n - tot, BSIZE - off%BSIZE);
80101b43:	39 d9                	cmp    %ebx,%ecx
80101b45:	0f 46 d9             	cmovbe %ecx,%ebx
    memmove(dst, bp->data + off%BSIZE, m);
80101b48:	83 c4 0c             	add    $0xc,%esp
80101b4b:	53                   	push   %ebx
  for(tot=0; tot<n; tot+=m, off+=m, dst+=m){
80101b4c:	01 de                	add    %ebx,%esi
80101b4e:	01 df                	add    %ebx,%edi
    memmove(dst, bp->data + off%BSIZE, m);
80101b50:	89 55 dc             	mov    %edx,-0x24(%ebp)
80101b53:	50                   	push   %eax
80101b54:	ff 75 e0             	push   -0x20(%ebp)
80101b57:	e8 d4 2b 00 00       	call   80104730 <memmove>
    brelse(bp);
80101b5c:	8b 55 dc             	mov    -0x24(%ebp),%edx
80101b5f:	89 14 24             	mov    %edx,(%esp)
80101b62:	e8 89 e6 ff ff       	call   801001f0 <brelse>
  for(tot=0; tot<n; tot+=m, off+=m, dst+=m){
80101b67:	01 5d e0             	add    %ebx,-0x20(%ebp)
80101b6a:	8b 5d e4             	mov    -0x1c(%ebp),%ebx
80101b6d:	83 c4 10             	add    $0x10,%esp
80101b70:	39 de                	cmp    %ebx,%esi
80101b72:	72 9c                	jb     80101b10 <readi+0x60>
80101b74:	89 d8                	mov    %ebx,%eax
  }
  return n;
}
80101b76:	8d 65 f4             	lea    -0xc(%ebp),%esp
80101b79:	5b                   	pop    %ebx
80101b7a:	5e                   	pop    %esi
80101b7b:	5f                   	pop    %edi
80101b7c:	5d                   	pop    %ebp
80101b7d:	c3                   	ret
80101b7e:	66 90                	xchg   %ax,%ax
    if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].read)
80101b80:	0f bf 56 52          	movswl 0x52(%esi),%edx
80101b84:	66 83 fa 09          	cmp    $0x9,%dx
80101b88:	77 17                	ja     80101ba1 <readi+0xf1>
80101b8a:	8b 14 d5 00 f9 10 80 	mov    -0x7fef0700(,%edx,8),%edx
80101b91:	85 d2                	test   %edx,%edx
80101b93:	74 0c                	je     80101ba1 <readi+0xf1>
    return devsw[ip->major].read(ip, dst, n);
80101b95:	89 45 10             	mov    %eax,0x10(%ebp)
}
80101b98:	8d 65 f4             	lea    -0xc(%ebp),%esp
80101b9b:	5b                   	pop    %ebx
80101b9c:	5e                   	pop    %esi
80101b9d:	5f                   	pop    %edi
80101b9e:	5d                   	pop    %ebp
    return devsw[ip->major].read(ip, dst, n);
80101b9f:	ff e2                	jmp    *%edx
      return -1;
80101ba1:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
80101ba6:	eb ce                	jmp    80101b76 <readi+0xc6>
80101ba8:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80101baf:	00 

80101bb0 <writei>:
// PAGEBREAK!
// Write data to inode.
// Caller must hold ip->lock.
int
writei(struct inode *ip, char *src, uint off, uint n)
{
80101bb0:	55                   	push   %ebp
80101bb1:	89 e5                	mov    %esp,%ebp
80101bb3:	57                   	push   %edi
80101bb4:	56                   	push   %esi
80101bb5:	53                   	push   %ebx
80101bb6:	83 ec 1c             	sub    $0x1c,%esp
80101bb9:	8b 45 08             	mov    0x8(%ebp),%eax
80101bbc:	8b 7d 0c             	mov    0xc(%ebp),%edi
80101bbf:	8b 75 14             	mov    0x14(%ebp),%esi
  uint tot, m;
  struct buf *bp;

  if(ip->type == T_DEV){
80101bc2:	66 83 78 50 03       	cmpw   $0x3,0x50(%eax)
{
80101bc7:	89 7d dc             	mov    %edi,-0x24(%ebp)
80101bca:	89 75 e0             	mov    %esi,-0x20(%ebp)
80101bcd:	8b 7d 10             	mov    0x10(%ebp),%edi
  if(ip->type == T_DEV){
80101bd0:	0f 84 ba 00 00 00    	je     80101c90 <writei+0xe0>
    if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].write)
      return -1;
    return devsw[ip->major].write(ip, src, n);
  }

  if(off > ip->size || off + n < off)
80101bd6:	39 78 58             	cmp    %edi,0x58(%eax)
80101bd9:	0f 82 ea 00 00 00    	jb     80101cc9 <writei+0x119>
    return -1;
  if(off + n > MAXFILE*BSIZE)
80101bdf:	8b 75 e0             	mov    -0x20(%ebp),%esi
80101be2:	89 f2                	mov    %esi,%edx
80101be4:	01 fa                	add    %edi,%edx
80101be6:	0f 82 dd 00 00 00    	jb     80101cc9 <writei+0x119>
80101bec:	81 fa 00 18 01 00    	cmp    $0x11800,%edx
80101bf2:	0f 87 d1 00 00 00    	ja     80101cc9 <writei+0x119>
    return -1;

  for(tot=0; tot<n; tot+=m, off+=m, src+=m){
80101bf8:	85 f6                	test   %esi,%esi
80101bfa:	0f 84 85 00 00 00    	je     80101c85 <writei+0xd5>
80101c00:	c7 45 e4 00 00 00 00 	movl   $0x0,-0x1c(%ebp)
    bp = bread(ip->dev, bmap(ip, off/BSIZE));
    m = min(n - tot, BSIZE - off%BSIZE);
80101c07:	89 45 d8             	mov    %eax,-0x28(%ebp)
80101c0a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    bp = bread(ip->dev, bmap(ip, off/BSIZE));
80101c10:	8b 75 d8             	mov    -0x28(%ebp),%esi
80101c13:	89 fa                	mov    %edi,%edx
80101c15:	c1 ea 09             	shr    $0x9,%edx
80101c18:	89 f0                	mov    %esi,%eax
80101c1a:	e8 51 f8 ff ff       	call   80101470 <bmap>
80101c1f:	83 ec 08             	sub    $0x8,%esp
80101c22:	50                   	push   %eax
80101c23:	ff 36                	push   (%esi)
80101c25:	e8 a6 e4 ff ff       	call   801000d0 <bread>
    m = min(n - tot, BSIZE - off%BSIZE);
80101c2a:	8b 55 e4             	mov    -0x1c(%ebp),%edx
80101c2d:	8b 5d e0             	mov    -0x20(%ebp),%ebx
80101c30:	b9 00 02 00 00       	mov    $0x200,%ecx
    bp = bread(ip->dev, bmap(ip, off/BSIZE));
80101c35:	89 c6                	mov    %eax,%esi
    m = min(n - tot, BSIZE - off%BSIZE);
80101c37:	89 f8                	mov    %edi,%eax
80101c39:	25 ff 01 00 00       	and    $0x1ff,%eax
80101c3e:	29 d3                	sub    %edx,%ebx
80101c40:	29 c1                	sub    %eax,%ecx
    memmove(bp->data + off%BSIZE, src, m);
80101c42:	8d 44 06 5c          	lea    0x5c(%esi,%eax,1),%eax
    m = min(n - tot, BSIZE - off%BSIZE);
80101c46:	39 d9                	cmp    %ebx,%ecx
80101c48:	0f 46 d9             	cmovbe %ecx,%ebx
    memmove(bp->data + off%BSIZE, src, m);
80101c4b:	83 c4 0c             	add    $0xc,%esp
80101c4e:	53                   	push   %ebx
  for(tot=0; tot<n; tot+=m, off+=m, src+=m){
80101c4f:	01 df                	add    %ebx,%edi
    memmove(bp->data + off%BSIZE, src, m);
80101c51:	ff 75 dc             	push   -0x24(%ebp)
80101c54:	50                   	push   %eax
80101c55:	e8 d6 2a 00 00       	call   80104730 <memmove>
    log_write(bp);
80101c5a:	89 34 24             	mov    %esi,(%esp)
80101c5d:	e8 be 12 00 00       	call   80102f20 <log_write>
    brelse(bp);
80101c62:	89 34 24             	mov    %esi,(%esp)
80101c65:	e8 86 e5 ff ff       	call   801001f0 <brelse>
  for(tot=0; tot<n; tot+=m, off+=m, src+=m){
80101c6a:	01 5d e4             	add    %ebx,-0x1c(%ebp)
80101c6d:	8b 45 e4             	mov    -0x1c(%ebp),%eax
80101c70:	83 c4 10             	add    $0x10,%esp
80101c73:	01 5d dc             	add    %ebx,-0x24(%ebp)
80101c76:	8b 5d e0             	mov    -0x20(%ebp),%ebx
80101c79:	39 d8                	cmp    %ebx,%eax
80101c7b:	72 93                	jb     80101c10 <writei+0x60>
  }

  if(n > 0 && off > ip->size){
80101c7d:	8b 45 d8             	mov    -0x28(%ebp),%eax
80101c80:	39 78 58             	cmp    %edi,0x58(%eax)
80101c83:	72 33                	jb     80101cb8 <writei+0x108>
    ip->size = off;
    iupdate(ip);
  }
  return n;
80101c85:	8b 45 e0             	mov    -0x20(%ebp),%eax
}
80101c88:	8d 65 f4             	lea    -0xc(%ebp),%esp
80101c8b:	5b                   	pop    %ebx
80101c8c:	5e                   	pop    %esi
80101c8d:	5f                   	pop    %edi
80101c8e:	5d                   	pop    %ebp
80101c8f:	c3                   	ret
    if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].write)
80101c90:	0f bf 40 52          	movswl 0x52(%eax),%eax
80101c94:	66 83 f8 09          	cmp    $0x9,%ax
80101c98:	77 2f                	ja     80101cc9 <writei+0x119>
80101c9a:	8b 04 c5 04 f9 10 80 	mov    -0x7fef06fc(,%eax,8),%eax
80101ca1:	85 c0                	test   %eax,%eax
80101ca3:	74 24                	je     80101cc9 <writei+0x119>
    return devsw[ip->major].write(ip, src, n);
80101ca5:	89 75 10             	mov    %esi,0x10(%ebp)
}
80101ca8:	8d 65 f4             	lea    -0xc(%ebp),%esp
80101cab:	5b                   	pop    %ebx
80101cac:	5e                   	pop    %esi
80101cad:	5f                   	pop    %edi
80101cae:	5d                   	pop    %ebp
    return devsw[ip->major].write(ip, src, n);
80101caf:	ff e0                	jmp    *%eax
80101cb1:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    iupdate(ip);
80101cb8:	83 ec 0c             	sub    $0xc,%esp
    ip->size = off;
80101cbb:	89 78 58             	mov    %edi,0x58(%eax)
    iupdate(ip);
80101cbe:	50                   	push   %eax
80101cbf:	e8 2c fa ff ff       	call   801016f0 <iupdate>
80101cc4:	83 c4 10             	add    $0x10,%esp
80101cc7:	eb bc                	jmp    80101c85 <writei+0xd5>
      return -1;
80101cc9:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
80101cce:	eb b8                	jmp    80101c88 <writei+0xd8>

80101cd0 <namecmp>:
//PAGEBREAK!
// Directories

int
namecmp(const char *s, const char *t)
{
80101cd0:	55                   	push   %ebp
80101cd1:	89 e5                	mov    %esp,%ebp
80101cd3:	83 ec 0c             	sub    $0xc,%esp
  return strncmp(s, t, DIRSIZ);
80101cd6:	6a 0e                	push   $0xe
80101cd8:	ff 75 0c             	push   0xc(%ebp)
80101cdb:	ff 75 08             	push   0x8(%ebp)
80101cde:	e8 bd 2a 00 00       	call   801047a0 <strncmp>
}
80101ce3:	c9                   	leave
80101ce4:	c3                   	ret
80101ce5:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80101cec:	00 
80101ced:	8d 76 00             	lea    0x0(%esi),%esi

80101cf0 <dirlookup>:

// Look for a directory entry in a directory.
// If found, set *poff to byte offset of entry.
struct inode*
dirlookup(struct inode *dp, char *name, uint *poff)
{
80101cf0:	55                   	push   %ebp
80101cf1:	89 e5                	mov    %esp,%ebp
80101cf3:	57                   	push   %edi
80101cf4:	56                   	push   %esi
80101cf5:	53                   	push   %ebx
80101cf6:	83 ec 1c             	sub    $0x1c,%esp
80101cf9:	8b 5d 08             	mov    0x8(%ebp),%ebx
  uint off, inum;
  struct dirent de;

  if(dp->type != T_DIR)
80101cfc:	66 83 7b 50 01       	cmpw   $0x1,0x50(%ebx)
80101d01:	0f 85 85 00 00 00    	jne    80101d8c <dirlookup+0x9c>
    panic("dirlookup not DIR");

  for(off = 0; off < dp->size; off += sizeof(de)){
80101d07:	8b 53 58             	mov    0x58(%ebx),%edx
80101d0a:	31 ff                	xor    %edi,%edi
80101d0c:	8d 75 d8             	lea    -0x28(%ebp),%esi
80101d0f:	85 d2                	test   %edx,%edx
80101d11:	74 3e                	je     80101d51 <dirlookup+0x61>
80101d13:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
    if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
80101d18:	6a 10                	push   $0x10
80101d1a:	57                   	push   %edi
80101d1b:	56                   	push   %esi
80101d1c:	53                   	push   %ebx
80101d1d:	e8 8e fd ff ff       	call   80101ab0 <readi>
80101d22:	83 c4 10             	add    $0x10,%esp
80101d25:	83 f8 10             	cmp    $0x10,%eax
80101d28:	75 55                	jne    80101d7f <dirlookup+0x8f>
      panic("dirlookup read");
    if(de.inum == 0)
80101d2a:	66 83 7d d8 00       	cmpw   $0x0,-0x28(%ebp)
80101d2f:	74 18                	je     80101d49 <dirlookup+0x59>
  return strncmp(s, t, DIRSIZ);
80101d31:	83 ec 04             	sub    $0x4,%esp
80101d34:	8d 45 da             	lea    -0x26(%ebp),%eax
80101d37:	6a 0e                	push   $0xe
80101d39:	50                   	push   %eax
80101d3a:	ff 75 0c             	push   0xc(%ebp)
80101d3d:	e8 5e 2a 00 00       	call   801047a0 <strncmp>
      continue;
    if(namecmp(name, de.name) == 0){
80101d42:	83 c4 10             	add    $0x10,%esp
80101d45:	85 c0                	test   %eax,%eax
80101d47:	74 17                	je     80101d60 <dirlookup+0x70>
  for(off = 0; off < dp->size; off += sizeof(de)){
80101d49:	83 c7 10             	add    $0x10,%edi
80101d4c:	3b 7b 58             	cmp    0x58(%ebx),%edi
80101d4f:	72 c7                	jb     80101d18 <dirlookup+0x28>
      return iget(dp->dev, inum);
    }
  }

  return 0;
}
80101d51:	8d 65 f4             	lea    -0xc(%ebp),%esp
  return 0;
80101d54:	31 c0                	xor    %eax,%eax
}
80101d56:	5b                   	pop    %ebx
80101d57:	5e                   	pop    %esi
80101d58:	5f                   	pop    %edi
80101d59:	5d                   	pop    %ebp
80101d5a:	c3                   	ret
80101d5b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
      if(poff)
80101d60:	8b 45 10             	mov    0x10(%ebp),%eax
80101d63:	85 c0                	test   %eax,%eax
80101d65:	74 05                	je     80101d6c <dirlookup+0x7c>
        *poff = off;
80101d67:	8b 45 10             	mov    0x10(%ebp),%eax
80101d6a:	89 38                	mov    %edi,(%eax)
      inum = de.inum;
80101d6c:	0f b7 55 d8          	movzwl -0x28(%ebp),%edx
      return iget(dp->dev, inum);
80101d70:	8b 03                	mov    (%ebx),%eax
80101d72:	e8 79 f5 ff ff       	call   801012f0 <iget>
}
80101d77:	8d 65 f4             	lea    -0xc(%ebp),%esp
80101d7a:	5b                   	pop    %ebx
80101d7b:	5e                   	pop    %esi
80101d7c:	5f                   	pop    %edi
80101d7d:	5d                   	pop    %ebp
80101d7e:	c3                   	ret
      panic("dirlookup read");
80101d7f:	83 ec 0c             	sub    $0xc,%esp
80101d82:	68 a8 72 10 80       	push   $0x801072a8
80101d87:	e8 f4 e5 ff ff       	call   80100380 <panic>
    panic("dirlookup not DIR");
80101d8c:	83 ec 0c             	sub    $0xc,%esp
80101d8f:	68 96 72 10 80       	push   $0x80107296
80101d94:	e8 e7 e5 ff ff       	call   80100380 <panic>
80101d99:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi

80101da0 <namex>:
// If parent != 0, return the inode for the parent and copy the final
// path element into name, which must have room for DIRSIZ bytes.
// Must be called inside a transaction since it calls iput().
static struct inode*
namex(char *path, int nameiparent, char *name)
{
80101da0:	55                   	push   %ebp
80101da1:	89 e5                	mov    %esp,%ebp
80101da3:	57                   	push   %edi
80101da4:	56                   	push   %esi
80101da5:	53                   	push   %ebx
80101da6:	89 c3                	mov    %eax,%ebx
80101da8:	83 ec 1c             	sub    $0x1c,%esp
  struct inode *ip, *next;

  if(*path == '/')
80101dab:	80 38 2f             	cmpb   $0x2f,(%eax)
{
80101dae:	89 55 dc             	mov    %edx,-0x24(%ebp)
80101db1:	89 4d e4             	mov    %ecx,-0x1c(%ebp)
  if(*path == '/')
80101db4:	0f 84 9e 01 00 00    	je     80101f58 <namex+0x1b8>
    ip = iget(ROOTDEV, ROOTINO);
  else
    ip = idup(myproc()->cwd);
80101dba:	e8 a1 1b 00 00       	call   80103960 <myproc>
  acquire(&icache.lock);
80101dbf:	83 ec 0c             	sub    $0xc,%esp
    ip = idup(myproc()->cwd);
80101dc2:	8b 70 68             	mov    0x68(%eax),%esi
  acquire(&icache.lock);
80101dc5:	68 60 f9 10 80       	push   $0x8010f960
80101dca:	e8 d1 27 00 00       	call   801045a0 <acquire>
  ip->ref++;
80101dcf:	83 46 08 01          	addl   $0x1,0x8(%esi)
  release(&icache.lock);
80101dd3:	c7 04 24 60 f9 10 80 	movl   $0x8010f960,(%esp)
80101dda:	e8 61 27 00 00       	call   80104540 <release>
80101ddf:	83 c4 10             	add    $0x10,%esp
80101de2:	eb 07                	jmp    80101deb <namex+0x4b>
80101de4:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    path++;
80101de8:	83 c3 01             	add    $0x1,%ebx
  while(*path == '/')
80101deb:	0f b6 03             	movzbl (%ebx),%eax
80101dee:	3c 2f                	cmp    $0x2f,%al
80101df0:	74 f6                	je     80101de8 <namex+0x48>
  if(*path == 0)
80101df2:	84 c0                	test   %al,%al
80101df4:	0f 84 06 01 00 00    	je     80101f00 <namex+0x160>
  while(*path != '/' && *path != 0)
80101dfa:	0f b6 03             	movzbl (%ebx),%eax
80101dfd:	84 c0                	test   %al,%al
80101dff:	0f 84 10 01 00 00    	je     80101f15 <namex+0x175>
80101e05:	89 df                	mov    %ebx,%edi
80101e07:	3c 2f                	cmp    $0x2f,%al
80101e09:	0f 84 06 01 00 00    	je     80101f15 <namex+0x175>
80101e0f:	90                   	nop
80101e10:	0f b6 47 01          	movzbl 0x1(%edi),%eax
    path++;
80101e14:	83 c7 01             	add    $0x1,%edi
  while(*path != '/' && *path != 0)
80101e17:	3c 2f                	cmp    $0x2f,%al
80101e19:	74 04                	je     80101e1f <namex+0x7f>
80101e1b:	84 c0                	test   %al,%al
80101e1d:	75 f1                	jne    80101e10 <namex+0x70>
  len = path - s;
80101e1f:	89 f8                	mov    %edi,%eax
80101e21:	29 d8                	sub    %ebx,%eax
  if(len >= DIRSIZ)
80101e23:	83 f8 0d             	cmp    $0xd,%eax
80101e26:	0f 8e ac 00 00 00    	jle    80101ed8 <namex+0x138>
    memmove(name, s, DIRSIZ);
80101e2c:	83 ec 04             	sub    $0x4,%esp
80101e2f:	6a 0e                	push   $0xe
80101e31:	53                   	push   %ebx
80101e32:	89 fb                	mov    %edi,%ebx
80101e34:	ff 75 e4             	push   -0x1c(%ebp)
80101e37:	e8 f4 28 00 00       	call   80104730 <memmove>
80101e3c:	83 c4 10             	add    $0x10,%esp
  while(*path == '/')
80101e3f:	80 3f 2f             	cmpb   $0x2f,(%edi)
80101e42:	75 0c                	jne    80101e50 <namex+0xb0>
80101e44:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    path++;
80101e48:	83 c3 01             	add    $0x1,%ebx
  while(*path == '/')
80101e4b:	80 3b 2f             	cmpb   $0x2f,(%ebx)
80101e4e:	74 f8                	je     80101e48 <namex+0xa8>

  while((path = skipelem(path, name)) != 0){
    ilock(ip);
80101e50:	83 ec 0c             	sub    $0xc,%esp
80101e53:	56                   	push   %esi
80101e54:	e8 47 f9 ff ff       	call   801017a0 <ilock>
    if(ip->type != T_DIR){
80101e59:	83 c4 10             	add    $0x10,%esp
80101e5c:	66 83 7e 50 01       	cmpw   $0x1,0x50(%esi)
80101e61:	0f 85 b7 00 00 00    	jne    80101f1e <namex+0x17e>
      iunlockput(ip);
      return 0;
    }
    if(nameiparent && *path == '\0'){
80101e67:	8b 45 dc             	mov    -0x24(%ebp),%eax
80101e6a:	85 c0                	test   %eax,%eax
80101e6c:	74 09                	je     80101e77 <namex+0xd7>
80101e6e:	80 3b 00             	cmpb   $0x0,(%ebx)
80101e71:	0f 84 f7 00 00 00    	je     80101f6e <namex+0x1ce>
      // Stop one level early.
      iunlock(ip);
      return ip;
    }
    if((next = dirlookup(ip, name, 0)) == 0){
80101e77:	83 ec 04             	sub    $0x4,%esp
80101e7a:	6a 00                	push   $0x0
80101e7c:	ff 75 e4             	push   -0x1c(%ebp)
80101e7f:	56                   	push   %esi
80101e80:	e8 6b fe ff ff       	call   80101cf0 <dirlookup>
80101e85:	83 c4 10             	add    $0x10,%esp
80101e88:	89 c7                	mov    %eax,%edi
80101e8a:	85 c0                	test   %eax,%eax
80101e8c:	0f 84 8c 00 00 00    	je     80101f1e <namex+0x17e>
  if(ip == 0 || !holdingsleep(&ip->lock) || ip->ref < 1)
80101e92:	83 ec 0c             	sub    $0xc,%esp
80101e95:	8d 4e 0c             	lea    0xc(%esi),%ecx
80101e98:	51                   	push   %ecx
80101e99:	89 4d e0             	mov    %ecx,-0x20(%ebp)
80101e9c:	e8 bf 24 00 00       	call   80104360 <holdingsleep>
80101ea1:	83 c4 10             	add    $0x10,%esp
80101ea4:	85 c0                	test   %eax,%eax
80101ea6:	0f 84 02 01 00 00    	je     80101fae <namex+0x20e>
80101eac:	8b 56 08             	mov    0x8(%esi),%edx
80101eaf:	85 d2                	test   %edx,%edx
80101eb1:	0f 8e f7 00 00 00    	jle    80101fae <namex+0x20e>
  releasesleep(&ip->lock);
80101eb7:	8b 4d e0             	mov    -0x20(%ebp),%ecx
80101eba:	83 ec 0c             	sub    $0xc,%esp
80101ebd:	51                   	push   %ecx
80101ebe:	e8 5d 24 00 00       	call   80104320 <releasesleep>
  iput(ip);
80101ec3:	89 34 24             	mov    %esi,(%esp)
      iunlockput(ip);
      return 0;
    }
    iunlockput(ip);
    ip = next;
80101ec6:	89 fe                	mov    %edi,%esi
  iput(ip);
80101ec8:	e8 03 fa ff ff       	call   801018d0 <iput>
80101ecd:	83 c4 10             	add    $0x10,%esp
  while(*path == '/')
80101ed0:	e9 16 ff ff ff       	jmp    80101deb <namex+0x4b>
80101ed5:	8d 76 00             	lea    0x0(%esi),%esi
    name[len] = 0;
80101ed8:	8b 55 e4             	mov    -0x1c(%ebp),%edx
80101edb:	8d 0c 02             	lea    (%edx,%eax,1),%ecx
    memmove(name, s, len);
80101ede:	83 ec 04             	sub    $0x4,%esp
80101ee1:	89 4d e0             	mov    %ecx,-0x20(%ebp)
80101ee4:	50                   	push   %eax
80101ee5:	53                   	push   %ebx
    name[len] = 0;
80101ee6:	89 fb                	mov    %edi,%ebx
    memmove(name, s, len);
80101ee8:	ff 75 e4             	push   -0x1c(%ebp)
80101eeb:	e8 40 28 00 00       	call   80104730 <memmove>
    name[len] = 0;
80101ef0:	8b 4d e0             	mov    -0x20(%ebp),%ecx
80101ef3:	83 c4 10             	add    $0x10,%esp
80101ef6:	c6 01 00             	movb   $0x0,(%ecx)
80101ef9:	e9 41 ff ff ff       	jmp    80101e3f <namex+0x9f>
80101efe:	66 90                	xchg   %ax,%ax
  }
  if(nameiparent){
80101f00:	8b 45 dc             	mov    -0x24(%ebp),%eax
80101f03:	85 c0                	test   %eax,%eax
80101f05:	0f 85 93 00 00 00    	jne    80101f9e <namex+0x1fe>
    iput(ip);
    return 0;
  }
  return ip;
}
80101f0b:	8d 65 f4             	lea    -0xc(%ebp),%esp
80101f0e:	89 f0                	mov    %esi,%eax
80101f10:	5b                   	pop    %ebx
80101f11:	5e                   	pop    %esi
80101f12:	5f                   	pop    %edi
80101f13:	5d                   	pop    %ebp
80101f14:	c3                   	ret
  while(*path != '/' && *path != 0)
80101f15:	8b 4d e4             	mov    -0x1c(%ebp),%ecx
80101f18:	89 df                	mov    %ebx,%edi
80101f1a:	31 c0                	xor    %eax,%eax
80101f1c:	eb c0                	jmp    80101ede <namex+0x13e>
  if(ip == 0 || !holdingsleep(&ip->lock) || ip->ref < 1)
80101f1e:	83 ec 0c             	sub    $0xc,%esp
80101f21:	8d 5e 0c             	lea    0xc(%esi),%ebx
80101f24:	53                   	push   %ebx
80101f25:	e8 36 24 00 00       	call   80104360 <holdingsleep>
80101f2a:	83 c4 10             	add    $0x10,%esp
80101f2d:	85 c0                	test   %eax,%eax
80101f2f:	74 7d                	je     80101fae <namex+0x20e>
80101f31:	8b 4e 08             	mov    0x8(%esi),%ecx
80101f34:	85 c9                	test   %ecx,%ecx
80101f36:	7e 76                	jle    80101fae <namex+0x20e>
  releasesleep(&ip->lock);
80101f38:	83 ec 0c             	sub    $0xc,%esp
80101f3b:	53                   	push   %ebx
80101f3c:	e8 df 23 00 00       	call   80104320 <releasesleep>
  iput(ip);
80101f41:	89 34 24             	mov    %esi,(%esp)
      return 0;
80101f44:	31 f6                	xor    %esi,%esi
  iput(ip);
80101f46:	e8 85 f9 ff ff       	call   801018d0 <iput>
      return 0;
80101f4b:	83 c4 10             	add    $0x10,%esp
}
80101f4e:	8d 65 f4             	lea    -0xc(%ebp),%esp
80101f51:	89 f0                	mov    %esi,%eax
80101f53:	5b                   	pop    %ebx
80101f54:	5e                   	pop    %esi
80101f55:	5f                   	pop    %edi
80101f56:	5d                   	pop    %ebp
80101f57:	c3                   	ret
    ip = iget(ROOTDEV, ROOTINO);
80101f58:	ba 01 00 00 00       	mov    $0x1,%edx
80101f5d:	b8 01 00 00 00       	mov    $0x1,%eax
80101f62:	e8 89 f3 ff ff       	call   801012f0 <iget>
80101f67:	89 c6                	mov    %eax,%esi
80101f69:	e9 7d fe ff ff       	jmp    80101deb <namex+0x4b>
  if(ip == 0 || !holdingsleep(&ip->lock) || ip->ref < 1)
80101f6e:	83 ec 0c             	sub    $0xc,%esp
80101f71:	8d 5e 0c             	lea    0xc(%esi),%ebx
80101f74:	53                   	push   %ebx
80101f75:	e8 e6 23 00 00       	call   80104360 <holdingsleep>
80101f7a:	83 c4 10             	add    $0x10,%esp
80101f7d:	85 c0                	test   %eax,%eax
80101f7f:	74 2d                	je     80101fae <namex+0x20e>
80101f81:	8b 7e 08             	mov    0x8(%esi),%edi
80101f84:	85 ff                	test   %edi,%edi
80101f86:	7e 26                	jle    80101fae <namex+0x20e>
  releasesleep(&ip->lock);
80101f88:	83 ec 0c             	sub    $0xc,%esp
80101f8b:	53                   	push   %ebx
80101f8c:	e8 8f 23 00 00       	call   80104320 <releasesleep>
}
80101f91:	83 c4 10             	add    $0x10,%esp
}
80101f94:	8d 65 f4             	lea    -0xc(%ebp),%esp
80101f97:	89 f0                	mov    %esi,%eax
80101f99:	5b                   	pop    %ebx
80101f9a:	5e                   	pop    %esi
80101f9b:	5f                   	pop    %edi
80101f9c:	5d                   	pop    %ebp
80101f9d:	c3                   	ret
    iput(ip);
80101f9e:	83 ec 0c             	sub    $0xc,%esp
80101fa1:	56                   	push   %esi
      return 0;
80101fa2:	31 f6                	xor    %esi,%esi
    iput(ip);
80101fa4:	e8 27 f9 ff ff       	call   801018d0 <iput>
    return 0;
80101fa9:	83 c4 10             	add    $0x10,%esp
80101fac:	eb a0                	jmp    80101f4e <namex+0x1ae>
    panic("iunlock");
80101fae:	83 ec 0c             	sub    $0xc,%esp
80101fb1:	68 8e 72 10 80       	push   $0x8010728e
80101fb6:	e8 c5 e3 ff ff       	call   80100380 <panic>
80101fbb:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi

80101fc0 <dirlink>:
{
80101fc0:	55                   	push   %ebp
80101fc1:	89 e5                	mov    %esp,%ebp
80101fc3:	57                   	push   %edi
80101fc4:	56                   	push   %esi
80101fc5:	53                   	push   %ebx
80101fc6:	83 ec 20             	sub    $0x20,%esp
80101fc9:	8b 5d 08             	mov    0x8(%ebp),%ebx
  if((ip = dirlookup(dp, name, 0)) != 0){
80101fcc:	6a 00                	push   $0x0
80101fce:	ff 75 0c             	push   0xc(%ebp)
80101fd1:	53                   	push   %ebx
80101fd2:	e8 19 fd ff ff       	call   80101cf0 <dirlookup>
80101fd7:	83 c4 10             	add    $0x10,%esp
80101fda:	85 c0                	test   %eax,%eax
80101fdc:	75 67                	jne    80102045 <dirlink+0x85>
  for(off = 0; off < dp->size; off += sizeof(de)){
80101fde:	8b 7b 58             	mov    0x58(%ebx),%edi
80101fe1:	8d 75 d8             	lea    -0x28(%ebp),%esi
80101fe4:	85 ff                	test   %edi,%edi
80101fe6:	74 29                	je     80102011 <dirlink+0x51>
80101fe8:	31 ff                	xor    %edi,%edi
80101fea:	8d 75 d8             	lea    -0x28(%ebp),%esi
80101fed:	eb 09                	jmp    80101ff8 <dirlink+0x38>
80101fef:	90                   	nop
80101ff0:	83 c7 10             	add    $0x10,%edi
80101ff3:	3b 7b 58             	cmp    0x58(%ebx),%edi
80101ff6:	73 19                	jae    80102011 <dirlink+0x51>
    if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
80101ff8:	6a 10                	push   $0x10
80101ffa:	57                   	push   %edi
80101ffb:	56                   	push   %esi
80101ffc:	53                   	push   %ebx
80101ffd:	e8 ae fa ff ff       	call   80101ab0 <readi>
80102002:	83 c4 10             	add    $0x10,%esp
80102005:	83 f8 10             	cmp    $0x10,%eax
80102008:	75 4e                	jne    80102058 <dirlink+0x98>
    if(de.inum == 0)
8010200a:	66 83 7d d8 00       	cmpw   $0x0,-0x28(%ebp)
8010200f:	75 df                	jne    80101ff0 <dirlink+0x30>
  strncpy(de.name, name, DIRSIZ);
80102011:	83 ec 04             	sub    $0x4,%esp
80102014:	8d 45 da             	lea    -0x26(%ebp),%eax
80102017:	6a 0e                	push   $0xe
80102019:	ff 75 0c             	push   0xc(%ebp)
8010201c:	50                   	push   %eax
8010201d:	e8 ce 27 00 00       	call   801047f0 <strncpy>
  de.inum = inum;
80102022:	8b 45 10             	mov    0x10(%ebp),%eax
80102025:	66 89 45 d8          	mov    %ax,-0x28(%ebp)
  if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
80102029:	6a 10                	push   $0x10
8010202b:	57                   	push   %edi
8010202c:	56                   	push   %esi
8010202d:	53                   	push   %ebx
8010202e:	e8 7d fb ff ff       	call   80101bb0 <writei>
80102033:	83 c4 20             	add    $0x20,%esp
80102036:	83 f8 10             	cmp    $0x10,%eax
80102039:	75 2a                	jne    80102065 <dirlink+0xa5>
  return 0;
8010203b:	31 c0                	xor    %eax,%eax
}
8010203d:	8d 65 f4             	lea    -0xc(%ebp),%esp
80102040:	5b                   	pop    %ebx
80102041:	5e                   	pop    %esi
80102042:	5f                   	pop    %edi
80102043:	5d                   	pop    %ebp
80102044:	c3                   	ret
    iput(ip);
80102045:	83 ec 0c             	sub    $0xc,%esp
80102048:	50                   	push   %eax
80102049:	e8 82 f8 ff ff       	call   801018d0 <iput>
    return -1;
8010204e:	83 c4 10             	add    $0x10,%esp
80102051:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
80102056:	eb e5                	jmp    8010203d <dirlink+0x7d>
      panic("dirlink read");
80102058:	83 ec 0c             	sub    $0xc,%esp
8010205b:	68 b7 72 10 80       	push   $0x801072b7
80102060:	e8 1b e3 ff ff       	call   80100380 <panic>
    panic("dirlink");
80102065:	83 ec 0c             	sub    $0xc,%esp
80102068:	68 13 75 10 80       	push   $0x80107513
8010206d:	e8 0e e3 ff ff       	call   80100380 <panic>
80102072:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80102079:	00 
8010207a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi

80102080 <namei>:

struct inode*
namei(char *path)
{
80102080:	55                   	push   %ebp
  char name[DIRSIZ];
  return namex(path, 0, name);
80102081:	31 d2                	xor    %edx,%edx
{
80102083:	89 e5                	mov    %esp,%ebp
80102085:	83 ec 18             	sub    $0x18,%esp
  return namex(path, 0, name);
80102088:	8b 45 08             	mov    0x8(%ebp),%eax
8010208b:	8d 4d ea             	lea    -0x16(%ebp),%ecx
8010208e:	e8 0d fd ff ff       	call   80101da0 <namex>
}
80102093:	c9                   	leave
80102094:	c3                   	ret
80102095:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010209c:	00 
8010209d:	8d 76 00             	lea    0x0(%esi),%esi

801020a0 <nameiparent>:

struct inode*
nameiparent(char *path, char *name)
{
801020a0:	55                   	push   %ebp
  return namex(path, 1, name);
801020a1:	ba 01 00 00 00       	mov    $0x1,%edx
{
801020a6:	89 e5                	mov    %esp,%ebp
  return namex(path, 1, name);
801020a8:	8b 4d 0c             	mov    0xc(%ebp),%ecx
801020ab:	8b 45 08             	mov    0x8(%ebp),%eax
}
801020ae:	5d                   	pop    %ebp
  return namex(path, 1, name);
801020af:	e9 ec fc ff ff       	jmp    80101da0 <namex>
801020b4:	66 90                	xchg   %ax,%ax
801020b6:	66 90                	xchg   %ax,%ax
801020b8:	66 90                	xchg   %ax,%ax
801020ba:	66 90                	xchg   %ax,%ax
801020bc:	66 90                	xchg   %ax,%ax
801020be:	66 90                	xchg   %ax,%ax

801020c0 <idestart>:
}

// Start the request for b.  Caller must hold idelock.
static void
idestart(struct buf *b)
{
801020c0:	55                   	push   %ebp
801020c1:	89 e5                	mov    %esp,%ebp
801020c3:	57                   	push   %edi
801020c4:	56                   	push   %esi
801020c5:	53                   	push   %ebx
801020c6:	83 ec 0c             	sub    $0xc,%esp
  if(b == 0)
801020c9:	85 c0                	test   %eax,%eax
801020cb:	0f 84 b4 00 00 00    	je     80102185 <idestart+0xc5>
    panic("idestart");
  if(b->blockno >= FSSIZE)
801020d1:	8b 70 08             	mov    0x8(%eax),%esi
801020d4:	89 c3                	mov    %eax,%ebx
801020d6:	81 fe e7 03 00 00    	cmp    $0x3e7,%esi
801020dc:	0f 87 96 00 00 00    	ja     80102178 <idestart+0xb8>
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
801020e2:	b9 f7 01 00 00       	mov    $0x1f7,%ecx
801020e7:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801020ee:	00 
801020ef:	90                   	nop
801020f0:	89 ca                	mov    %ecx,%edx
801020f2:	ec                   	in     (%dx),%al
  while(((r = inb(0x1f7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
801020f3:	83 e0 c0             	and    $0xffffffc0,%eax
801020f6:	3c 40                	cmp    $0x40,%al
801020f8:	75 f6                	jne    801020f0 <idestart+0x30>
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
801020fa:	31 ff                	xor    %edi,%edi
801020fc:	ba f6 03 00 00       	mov    $0x3f6,%edx
80102101:	89 f8                	mov    %edi,%eax
80102103:	ee                   	out    %al,(%dx)
80102104:	b8 01 00 00 00       	mov    $0x1,%eax
80102109:	ba f2 01 00 00       	mov    $0x1f2,%edx
8010210e:	ee                   	out    %al,(%dx)
8010210f:	ba f3 01 00 00       	mov    $0x1f3,%edx
80102114:	89 f0                	mov    %esi,%eax
80102116:	ee                   	out    %al,(%dx)

  idewait(0);
  outb(0x3f6, 0);  // generate interrupt
  outb(0x1f2, sector_per_block);  // number of sectors
  outb(0x1f3, sector & 0xff);
  outb(0x1f4, (sector >> 8) & 0xff);
80102117:	89 f0                	mov    %esi,%eax
80102119:	ba f4 01 00 00       	mov    $0x1f4,%edx
8010211e:	c1 f8 08             	sar    $0x8,%eax
80102121:	ee                   	out    %al,(%dx)
80102122:	ba f5 01 00 00       	mov    $0x1f5,%edx
80102127:	89 f8                	mov    %edi,%eax
80102129:	ee                   	out    %al,(%dx)
  outb(0x1f5, (sector >> 16) & 0xff);
  outb(0x1f6, 0xe0 | ((b->dev&1)<<4) | ((sector>>24)&0x0f));
8010212a:	0f b6 43 04          	movzbl 0x4(%ebx),%eax
8010212e:	ba f6 01 00 00       	mov    $0x1f6,%edx
80102133:	c1 e0 04             	shl    $0x4,%eax
80102136:	83 e0 10             	and    $0x10,%eax
80102139:	83 c8 e0             	or     $0xffffffe0,%eax
8010213c:	ee                   	out    %al,(%dx)
  if(b->flags & B_DIRTY){
8010213d:	f6 03 04             	testb  $0x4,(%ebx)
80102140:	75 16                	jne    80102158 <idestart+0x98>
80102142:	b8 20 00 00 00       	mov    $0x20,%eax
80102147:	89 ca                	mov    %ecx,%edx
80102149:	ee                   	out    %al,(%dx)
    outb(0x1f7, write_cmd);
    outsl(0x1f0, b->data, BSIZE/4);
  } else {
    outb(0x1f7, read_cmd);
  }
}
8010214a:	8d 65 f4             	lea    -0xc(%ebp),%esp
8010214d:	5b                   	pop    %ebx
8010214e:	5e                   	pop    %esi
8010214f:	5f                   	pop    %edi
80102150:	5d                   	pop    %ebp
80102151:	c3                   	ret
80102152:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
80102158:	b8 30 00 00 00       	mov    $0x30,%eax
8010215d:	89 ca                	mov    %ecx,%edx
8010215f:	ee                   	out    %al,(%dx)
  asm volatile("cld; rep outsl" :
80102160:	b9 80 00 00 00       	mov    $0x80,%ecx
    outsl(0x1f0, b->data, BSIZE/4);
80102165:	8d 73 5c             	lea    0x5c(%ebx),%esi
80102168:	ba f0 01 00 00       	mov    $0x1f0,%edx
8010216d:	fc                   	cld
8010216e:	f3 6f                	rep outsl %ds:(%esi),(%dx)
}
80102170:	8d 65 f4             	lea    -0xc(%ebp),%esp
80102173:	5b                   	pop    %ebx
80102174:	5e                   	pop    %esi
80102175:	5f                   	pop    %edi
80102176:	5d                   	pop    %ebp
80102177:	c3                   	ret
    panic("incorrect blockno");
80102178:	83 ec 0c             	sub    $0xc,%esp
8010217b:	68 cd 72 10 80       	push   $0x801072cd
80102180:	e8 fb e1 ff ff       	call   80100380 <panic>
    panic("idestart");
80102185:	83 ec 0c             	sub    $0xc,%esp
80102188:	68 c4 72 10 80       	push   $0x801072c4
8010218d:	e8 ee e1 ff ff       	call   80100380 <panic>
80102192:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80102199:	00 
8010219a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi

801021a0 <ideinit>:
{
801021a0:	55                   	push   %ebp
801021a1:	89 e5                	mov    %esp,%ebp
801021a3:	83 ec 10             	sub    $0x10,%esp
  initlock(&idelock, "ide");
801021a6:	68 df 72 10 80       	push   $0x801072df
801021ab:	68 00 16 11 80       	push   $0x80111600
801021b0:	e8 fb 21 00 00       	call   801043b0 <initlock>
  ioapicenable(IRQ_IDE, ncpu - 1);
801021b5:	58                   	pop    %eax
801021b6:	a1 84 17 11 80       	mov    0x80111784,%eax
801021bb:	5a                   	pop    %edx
801021bc:	83 e8 01             	sub    $0x1,%eax
801021bf:	50                   	push   %eax
801021c0:	6a 0e                	push   $0xe
801021c2:	e8 99 02 00 00       	call   80102460 <ioapicenable>
  while(((r = inb(0x1f7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
801021c7:	83 c4 10             	add    $0x10,%esp
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
801021ca:	b9 f7 01 00 00       	mov    $0x1f7,%ecx
801021cf:	90                   	nop
801021d0:	89 ca                	mov    %ecx,%edx
801021d2:	ec                   	in     (%dx),%al
801021d3:	83 e0 c0             	and    $0xffffffc0,%eax
801021d6:	3c 40                	cmp    $0x40,%al
801021d8:	75 f6                	jne    801021d0 <ideinit+0x30>
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
801021da:	b8 f0 ff ff ff       	mov    $0xfffffff0,%eax
801021df:	ba f6 01 00 00       	mov    $0x1f6,%edx
801021e4:	ee                   	out    %al,(%dx)
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
801021e5:	89 ca                	mov    %ecx,%edx
801021e7:	ec                   	in     (%dx),%al
    if(inb(0x1f7) != 0){
801021e8:	84 c0                	test   %al,%al
801021ea:	75 1e                	jne    8010220a <ideinit+0x6a>
801021ec:	b9 e8 03 00 00       	mov    $0x3e8,%ecx
801021f1:	ba f7 01 00 00       	mov    $0x1f7,%edx
801021f6:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801021fd:	00 
801021fe:	66 90                	xchg   %ax,%ax
  for(i=0; i<1000; i++){
80102200:	83 e9 01             	sub    $0x1,%ecx
80102203:	74 0f                	je     80102214 <ideinit+0x74>
80102205:	ec                   	in     (%dx),%al
    if(inb(0x1f7) != 0){
80102206:	84 c0                	test   %al,%al
80102208:	74 f6                	je     80102200 <ideinit+0x60>
      havedisk1 = 1;
8010220a:	c7 05 e0 15 11 80 01 	movl   $0x1,0x801115e0
80102211:	00 00 00 
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
80102214:	b8 e0 ff ff ff       	mov    $0xffffffe0,%eax
80102219:	ba f6 01 00 00       	mov    $0x1f6,%edx
8010221e:	ee                   	out    %al,(%dx)
}
8010221f:	c9                   	leave
80102220:	c3                   	ret
80102221:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80102228:	00 
80102229:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi

80102230 <ideintr>:

// Interrupt handler.
void
ideintr(void)
{
80102230:	55                   	push   %ebp
80102231:	89 e5                	mov    %esp,%ebp
80102233:	57                   	push   %edi
80102234:	56                   	push   %esi
80102235:	53                   	push   %ebx
80102236:	83 ec 18             	sub    $0x18,%esp
  struct buf *b;

  // First queued buffer is the active request.
  acquire(&idelock);
80102239:	68 00 16 11 80       	push   $0x80111600
8010223e:	e8 5d 23 00 00       	call   801045a0 <acquire>

  if((b = idequeue) == 0){
80102243:	8b 1d e4 15 11 80    	mov    0x801115e4,%ebx
80102249:	83 c4 10             	add    $0x10,%esp
8010224c:	85 db                	test   %ebx,%ebx
8010224e:	74 63                	je     801022b3 <ideintr+0x83>
    release(&idelock);
    return;
  }
  idequeue = b->qnext;
80102250:	8b 43 58             	mov    0x58(%ebx),%eax
80102253:	a3 e4 15 11 80       	mov    %eax,0x801115e4

  // Read data if needed.
  if(!(b->flags & B_DIRTY) && idewait(1) >= 0)
80102258:	8b 33                	mov    (%ebx),%esi
8010225a:	f7 c6 04 00 00 00    	test   $0x4,%esi
80102260:	75 2f                	jne    80102291 <ideintr+0x61>
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
80102262:	ba f7 01 00 00       	mov    $0x1f7,%edx
80102267:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010226e:	00 
8010226f:	90                   	nop
80102270:	ec                   	in     (%dx),%al
  while(((r = inb(0x1f7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
80102271:	89 c1                	mov    %eax,%ecx
80102273:	83 e1 c0             	and    $0xffffffc0,%ecx
80102276:	80 f9 40             	cmp    $0x40,%cl
80102279:	75 f5                	jne    80102270 <ideintr+0x40>
  if(checkerr && (r & (IDE_DF|IDE_ERR)) != 0)
8010227b:	a8 21                	test   $0x21,%al
8010227d:	75 12                	jne    80102291 <ideintr+0x61>
    insl(0x1f0, b->data, BSIZE/4);
8010227f:	8d 7b 5c             	lea    0x5c(%ebx),%edi
  asm volatile("cld; rep insl" :
80102282:	b9 80 00 00 00       	mov    $0x80,%ecx
80102287:	ba f0 01 00 00       	mov    $0x1f0,%edx
8010228c:	fc                   	cld
8010228d:	f3 6d                	rep insl (%dx),%es:(%edi)

  // Wake process waiting for this buf.
  b->flags |= B_VALID;
8010228f:	8b 33                	mov    (%ebx),%esi
  b->flags &= ~B_DIRTY;
80102291:	83 e6 fb             	and    $0xfffffffb,%esi
  wakeup(b);
80102294:	83 ec 0c             	sub    $0xc,%esp
  b->flags &= ~B_DIRTY;
80102297:	83 ce 02             	or     $0x2,%esi
8010229a:	89 33                	mov    %esi,(%ebx)
  wakeup(b);
8010229c:	53                   	push   %ebx
8010229d:	e8 3e 1e 00 00       	call   801040e0 <wakeup>

  // Start disk on next buf in queue.
  if(idequeue != 0)
801022a2:	a1 e4 15 11 80       	mov    0x801115e4,%eax
801022a7:	83 c4 10             	add    $0x10,%esp
801022aa:	85 c0                	test   %eax,%eax
801022ac:	74 05                	je     801022b3 <ideintr+0x83>
    idestart(idequeue);
801022ae:	e8 0d fe ff ff       	call   801020c0 <idestart>
    release(&idelock);
801022b3:	83 ec 0c             	sub    $0xc,%esp
801022b6:	68 00 16 11 80       	push   $0x80111600
801022bb:	e8 80 22 00 00       	call   80104540 <release>

  release(&idelock);
}
801022c0:	8d 65 f4             	lea    -0xc(%ebp),%esp
801022c3:	5b                   	pop    %ebx
801022c4:	5e                   	pop    %esi
801022c5:	5f                   	pop    %edi
801022c6:	5d                   	pop    %ebp
801022c7:	c3                   	ret
801022c8:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801022cf:	00 

801022d0 <iderw>:
// Sync buf with disk.
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void
iderw(struct buf *b)
{
801022d0:	55                   	push   %ebp
801022d1:	89 e5                	mov    %esp,%ebp
801022d3:	53                   	push   %ebx
801022d4:	83 ec 10             	sub    $0x10,%esp
801022d7:	8b 5d 08             	mov    0x8(%ebp),%ebx
  struct buf **pp;

  if(!holdingsleep(&b->lock))
801022da:	8d 43 0c             	lea    0xc(%ebx),%eax
801022dd:	50                   	push   %eax
801022de:	e8 7d 20 00 00       	call   80104360 <holdingsleep>
801022e3:	83 c4 10             	add    $0x10,%esp
801022e6:	85 c0                	test   %eax,%eax
801022e8:	0f 84 c3 00 00 00    	je     801023b1 <iderw+0xe1>
    panic("iderw: buf not locked");
  if((b->flags & (B_VALID|B_DIRTY)) == B_VALID)
801022ee:	8b 03                	mov    (%ebx),%eax
801022f0:	83 e0 06             	and    $0x6,%eax
801022f3:	83 f8 02             	cmp    $0x2,%eax
801022f6:	0f 84 a8 00 00 00    	je     801023a4 <iderw+0xd4>
    panic("iderw: nothing to do");
  if(b->dev != 0 && !havedisk1)
801022fc:	8b 53 04             	mov    0x4(%ebx),%edx
801022ff:	85 d2                	test   %edx,%edx
80102301:	74 0d                	je     80102310 <iderw+0x40>
80102303:	a1 e0 15 11 80       	mov    0x801115e0,%eax
80102308:	85 c0                	test   %eax,%eax
8010230a:	0f 84 87 00 00 00    	je     80102397 <iderw+0xc7>
    panic("iderw: ide disk 1 not present");

  acquire(&idelock);  //DOC:acquire-lock
80102310:	83 ec 0c             	sub    $0xc,%esp
80102313:	68 00 16 11 80       	push   $0x80111600
80102318:	e8 83 22 00 00       	call   801045a0 <acquire>

  // Append b to idequeue.
  b->qnext = 0;
  for(pp=&idequeue; *pp; pp=&(*pp)->qnext)  //DOC:insert-queue
8010231d:	a1 e4 15 11 80       	mov    0x801115e4,%eax
  b->qnext = 0;
80102322:	c7 43 58 00 00 00 00 	movl   $0x0,0x58(%ebx)
  for(pp=&idequeue; *pp; pp=&(*pp)->qnext)  //DOC:insert-queue
80102329:	83 c4 10             	add    $0x10,%esp
8010232c:	85 c0                	test   %eax,%eax
8010232e:	74 60                	je     80102390 <iderw+0xc0>
80102330:	89 c2                	mov    %eax,%edx
80102332:	8b 40 58             	mov    0x58(%eax),%eax
80102335:	85 c0                	test   %eax,%eax
80102337:	75 f7                	jne    80102330 <iderw+0x60>
80102339:	83 c2 58             	add    $0x58,%edx
    ;
  *pp = b;
8010233c:	89 1a                	mov    %ebx,(%edx)

  // Start disk if necessary.
  if(idequeue == b)
8010233e:	39 1d e4 15 11 80    	cmp    %ebx,0x801115e4
80102344:	74 3a                	je     80102380 <iderw+0xb0>
    idestart(b);

  // Wait for request to finish.
  while((b->flags & (B_VALID|B_DIRTY)) != B_VALID){
80102346:	8b 03                	mov    (%ebx),%eax
80102348:	83 e0 06             	and    $0x6,%eax
8010234b:	83 f8 02             	cmp    $0x2,%eax
8010234e:	74 1b                	je     8010236b <iderw+0x9b>
    sleep(b, &idelock);
80102350:	83 ec 08             	sub    $0x8,%esp
80102353:	68 00 16 11 80       	push   $0x80111600
80102358:	53                   	push   %ebx
80102359:	e8 c2 1c 00 00       	call   80104020 <sleep>
  while((b->flags & (B_VALID|B_DIRTY)) != B_VALID){
8010235e:	8b 03                	mov    (%ebx),%eax
80102360:	83 c4 10             	add    $0x10,%esp
80102363:	83 e0 06             	and    $0x6,%eax
80102366:	83 f8 02             	cmp    $0x2,%eax
80102369:	75 e5                	jne    80102350 <iderw+0x80>
  }


  release(&idelock);
8010236b:	c7 45 08 00 16 11 80 	movl   $0x80111600,0x8(%ebp)
}
80102372:	8b 5d fc             	mov    -0x4(%ebp),%ebx
80102375:	c9                   	leave
  release(&idelock);
80102376:	e9 c5 21 00 00       	jmp    80104540 <release>
8010237b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
    idestart(b);
80102380:	89 d8                	mov    %ebx,%eax
80102382:	e8 39 fd ff ff       	call   801020c0 <idestart>
80102387:	eb bd                	jmp    80102346 <iderw+0x76>
80102389:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
  for(pp=&idequeue; *pp; pp=&(*pp)->qnext)  //DOC:insert-queue
80102390:	ba e4 15 11 80       	mov    $0x801115e4,%edx
80102395:	eb a5                	jmp    8010233c <iderw+0x6c>
    panic("iderw: ide disk 1 not present");
80102397:	83 ec 0c             	sub    $0xc,%esp
8010239a:	68 0e 73 10 80       	push   $0x8010730e
8010239f:	e8 dc df ff ff       	call   80100380 <panic>
    panic("iderw: nothing to do");
801023a4:	83 ec 0c             	sub    $0xc,%esp
801023a7:	68 f9 72 10 80       	push   $0x801072f9
801023ac:	e8 cf df ff ff       	call   80100380 <panic>
    panic("iderw: buf not locked");
801023b1:	83 ec 0c             	sub    $0xc,%esp
801023b4:	68 e3 72 10 80       	push   $0x801072e3
801023b9:	e8 c2 df ff ff       	call   80100380 <panic>
801023be:	66 90                	xchg   %ax,%ax

801023c0 <ioapicinit>:
  ioapic->data = data;
}

void
ioapicinit(void)
{
801023c0:	55                   	push   %ebp
801023c1:	89 e5                	mov    %esp,%ebp
801023c3:	56                   	push   %esi
801023c4:	53                   	push   %ebx
  int i, id, maxintr;

  ioapic = (volatile struct ioapic*)IOAPIC;
801023c5:	c7 05 34 16 11 80 00 	movl   $0xfec00000,0x80111634
801023cc:	00 c0 fe 
  ioapic->reg = reg;
801023cf:	c7 05 00 00 c0 fe 01 	movl   $0x1,0xfec00000
801023d6:	00 00 00 
  return ioapic->data;
801023d9:	8b 15 34 16 11 80    	mov    0x80111634,%edx
801023df:	8b 72 10             	mov    0x10(%edx),%esi
  ioapic->reg = reg;
801023e2:	c7 02 00 00 00 00    	movl   $0x0,(%edx)
  return ioapic->data;
801023e8:	8b 1d 34 16 11 80    	mov    0x80111634,%ebx
  maxintr = (ioapicread(REG_VER) >> 16) & 0xFF;
  id = ioapicread(REG_ID) >> 24;
  if(id != ioapicid)
801023ee:	0f b6 15 80 17 11 80 	movzbl 0x80111780,%edx
  maxintr = (ioapicread(REG_VER) >> 16) & 0xFF;
801023f5:	c1 ee 10             	shr    $0x10,%esi
801023f8:	89 f0                	mov    %esi,%eax
801023fa:	0f b6 f0             	movzbl %al,%esi
  return ioapic->data;
801023fd:	8b 43 10             	mov    0x10(%ebx),%eax
  id = ioapicread(REG_ID) >> 24;
80102400:	c1 e8 18             	shr    $0x18,%eax
  if(id != ioapicid)
80102403:	39 c2                	cmp    %eax,%edx
80102405:	74 16                	je     8010241d <ioapicinit+0x5d>
    cprintf("ioapicinit: id isn't equal to ioapicid; not a MP\n");
80102407:	83 ec 0c             	sub    $0xc,%esp
8010240a:	68 c8 76 10 80       	push   $0x801076c8
8010240f:	e8 9c e2 ff ff       	call   801006b0 <cprintf>
  ioapic->reg = reg;
80102414:	8b 1d 34 16 11 80    	mov    0x80111634,%ebx
8010241a:	83 c4 10             	add    $0x10,%esp
{
8010241d:	ba 10 00 00 00       	mov    $0x10,%edx
80102422:	31 c0                	xor    %eax,%eax
80102424:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
  ioapic->reg = reg;
80102428:	89 13                	mov    %edx,(%ebx)
8010242a:	8d 48 20             	lea    0x20(%eax),%ecx
  ioapic->data = data;
8010242d:	8b 1d 34 16 11 80    	mov    0x80111634,%ebx

  // Mark all interrupts edge-triggered, active high, disabled,
  // and not routed to any CPUs.
  for(i = 0; i <= maxintr; i++){
80102433:	83 c0 01             	add    $0x1,%eax
80102436:	81 c9 00 00 01 00    	or     $0x10000,%ecx
  ioapic->data = data;
8010243c:	89 4b 10             	mov    %ecx,0x10(%ebx)
  ioapic->reg = reg;
8010243f:	8d 4a 01             	lea    0x1(%edx),%ecx
  for(i = 0; i <= maxintr; i++){
80102442:	83 c2 02             	add    $0x2,%edx
  ioapic->reg = reg;
80102445:	89 0b                	mov    %ecx,(%ebx)
  ioapic->data = data;
80102447:	8b 1d 34 16 11 80    	mov    0x80111634,%ebx
8010244d:	c7 43 10 00 00 00 00 	movl   $0x0,0x10(%ebx)
  for(i = 0; i <= maxintr; i++){
80102454:	39 c6                	cmp    %eax,%esi
80102456:	7d d0                	jge    80102428 <ioapicinit+0x68>
    ioapicwrite(REG_TABLE+2*i, INT_DISABLED | (T_IRQ0 + i));
    ioapicwrite(REG_TABLE+2*i+1, 0);
  }
}
80102458:	8d 65 f8             	lea    -0x8(%ebp),%esp
8010245b:	5b                   	pop    %ebx
8010245c:	5e                   	pop    %esi
8010245d:	5d                   	pop    %ebp
8010245e:	c3                   	ret
8010245f:	90                   	nop

80102460 <ioapicenable>:

void
ioapicenable(int irq, int cpunum)
{
80102460:	55                   	push   %ebp
  ioapic->reg = reg;
80102461:	8b 0d 34 16 11 80    	mov    0x80111634,%ecx
{
80102467:	89 e5                	mov    %esp,%ebp
80102469:	8b 45 08             	mov    0x8(%ebp),%eax
  // Mark interrupt edge-triggered, active high,
  // enabled, and routed to the given cpunum,
  // which happens to be that cpu's APIC ID.
  ioapicwrite(REG_TABLE+2*irq, T_IRQ0 + irq);
8010246c:	8d 50 20             	lea    0x20(%eax),%edx
8010246f:	8d 44 00 10          	lea    0x10(%eax,%eax,1),%eax
  ioapic->reg = reg;
80102473:	89 01                	mov    %eax,(%ecx)
  ioapic->data = data;
80102475:	8b 0d 34 16 11 80    	mov    0x80111634,%ecx
  ioapicwrite(REG_TABLE+2*irq+1, cpunum << 24);
8010247b:	83 c0 01             	add    $0x1,%eax
  ioapic->data = data;
8010247e:	89 51 10             	mov    %edx,0x10(%ecx)
  ioapicwrite(REG_TABLE+2*irq+1, cpunum << 24);
80102481:	8b 55 0c             	mov    0xc(%ebp),%edx
  ioapic->reg = reg;
80102484:	89 01                	mov    %eax,(%ecx)
  ioapic->data = data;
80102486:	a1 34 16 11 80       	mov    0x80111634,%eax
  ioapicwrite(REG_TABLE+2*irq+1, cpunum << 24);
8010248b:	c1 e2 18             	shl    $0x18,%edx
  ioapic->data = data;
8010248e:	89 50 10             	mov    %edx,0x10(%eax)
}
80102491:	5d                   	pop    %ebp
80102492:	c3                   	ret
80102493:	66 90                	xchg   %ax,%ax
80102495:	66 90                	xchg   %ax,%ax
80102497:	66 90                	xchg   %ax,%ax
80102499:	66 90                	xchg   %ax,%ax
8010249b:	66 90                	xchg   %ax,%ax
8010249d:	66 90                	xchg   %ax,%ax
8010249f:	90                   	nop

801024a0 <kfree>:
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
801024a0:	55                   	push   %ebp
801024a1:	89 e5                	mov    %esp,%ebp
801024a3:	53                   	push   %ebx
801024a4:	83 ec 04             	sub    $0x4,%esp
801024a7:	8b 5d 08             	mov    0x8(%ebp),%ebx
  struct run *r;

  if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
801024aa:	f7 c3 ff 0f 00 00    	test   $0xfff,%ebx
801024b0:	75 76                	jne    80102528 <kfree+0x88>
801024b2:	81 fb d0 54 11 80    	cmp    $0x801154d0,%ebx
801024b8:	72 6e                	jb     80102528 <kfree+0x88>
801024ba:	8d 83 00 00 00 80    	lea    -0x80000000(%ebx),%eax
801024c0:	3d ff ff ff 0d       	cmp    $0xdffffff,%eax
801024c5:	77 61                	ja     80102528 <kfree+0x88>
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(v, 1, PGSIZE);
801024c7:	83 ec 04             	sub    $0x4,%esp
801024ca:	68 00 10 00 00       	push   $0x1000
801024cf:	6a 01                	push   $0x1
801024d1:	53                   	push   %ebx
801024d2:	e8 c9 21 00 00       	call   801046a0 <memset>

  if(kmem.use_lock)
801024d7:	8b 15 74 16 11 80    	mov    0x80111674,%edx
801024dd:	83 c4 10             	add    $0x10,%esp
801024e0:	85 d2                	test   %edx,%edx
801024e2:	75 1c                	jne    80102500 <kfree+0x60>
    acquire(&kmem.lock);
  r = (struct run*)v;
  r->next = kmem.freelist;
801024e4:	a1 78 16 11 80       	mov    0x80111678,%eax
801024e9:	89 03                	mov    %eax,(%ebx)
  kmem.freelist = r;
  if(kmem.use_lock)
801024eb:	a1 74 16 11 80       	mov    0x80111674,%eax
  kmem.freelist = r;
801024f0:	89 1d 78 16 11 80    	mov    %ebx,0x80111678
  if(kmem.use_lock)
801024f6:	85 c0                	test   %eax,%eax
801024f8:	75 1e                	jne    80102518 <kfree+0x78>
    release(&kmem.lock);
}
801024fa:	8b 5d fc             	mov    -0x4(%ebp),%ebx
801024fd:	c9                   	leave
801024fe:	c3                   	ret
801024ff:	90                   	nop
    acquire(&kmem.lock);
80102500:	83 ec 0c             	sub    $0xc,%esp
80102503:	68 40 16 11 80       	push   $0x80111640
80102508:	e8 93 20 00 00       	call   801045a0 <acquire>
8010250d:	83 c4 10             	add    $0x10,%esp
80102510:	eb d2                	jmp    801024e4 <kfree+0x44>
80102512:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    release(&kmem.lock);
80102518:	c7 45 08 40 16 11 80 	movl   $0x80111640,0x8(%ebp)
}
8010251f:	8b 5d fc             	mov    -0x4(%ebp),%ebx
80102522:	c9                   	leave
    release(&kmem.lock);
80102523:	e9 18 20 00 00       	jmp    80104540 <release>
    panic("kfree");
80102528:	83 ec 0c             	sub    $0xc,%esp
8010252b:	68 2c 73 10 80       	push   $0x8010732c
80102530:	e8 4b de ff ff       	call   80100380 <panic>
80102535:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010253c:	00 
8010253d:	8d 76 00             	lea    0x0(%esi),%esi

80102540 <freerange>:
{
80102540:	55                   	push   %ebp
80102541:	89 e5                	mov    %esp,%ebp
80102543:	56                   	push   %esi
80102544:	53                   	push   %ebx
  p = (char*)PGROUNDUP((uint)vstart);
80102545:	8b 45 08             	mov    0x8(%ebp),%eax
{
80102548:	8b 75 0c             	mov    0xc(%ebp),%esi
  p = (char*)PGROUNDUP((uint)vstart);
8010254b:	8d 98 ff 0f 00 00    	lea    0xfff(%eax),%ebx
80102551:	81 e3 00 f0 ff ff    	and    $0xfffff000,%ebx
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
80102557:	81 c3 00 10 00 00    	add    $0x1000,%ebx
8010255d:	39 de                	cmp    %ebx,%esi
8010255f:	72 23                	jb     80102584 <freerange+0x44>
80102561:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    kfree(p);
80102568:	83 ec 0c             	sub    $0xc,%esp
8010256b:	8d 83 00 f0 ff ff    	lea    -0x1000(%ebx),%eax
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
80102571:	81 c3 00 10 00 00    	add    $0x1000,%ebx
    kfree(p);
80102577:	50                   	push   %eax
80102578:	e8 23 ff ff ff       	call   801024a0 <kfree>
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
8010257d:	83 c4 10             	add    $0x10,%esp
80102580:	39 de                	cmp    %ebx,%esi
80102582:	73 e4                	jae    80102568 <freerange+0x28>
}
80102584:	8d 65 f8             	lea    -0x8(%ebp),%esp
80102587:	5b                   	pop    %ebx
80102588:	5e                   	pop    %esi
80102589:	5d                   	pop    %ebp
8010258a:	c3                   	ret
8010258b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi

80102590 <kinit2>:
{
80102590:	55                   	push   %ebp
80102591:	89 e5                	mov    %esp,%ebp
80102593:	56                   	push   %esi
80102594:	53                   	push   %ebx
  p = (char*)PGROUNDUP((uint)vstart);
80102595:	8b 45 08             	mov    0x8(%ebp),%eax
{
80102598:	8b 75 0c             	mov    0xc(%ebp),%esi
  p = (char*)PGROUNDUP((uint)vstart);
8010259b:	8d 98 ff 0f 00 00    	lea    0xfff(%eax),%ebx
801025a1:	81 e3 00 f0 ff ff    	and    $0xfffff000,%ebx
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
801025a7:	81 c3 00 10 00 00    	add    $0x1000,%ebx
801025ad:	39 de                	cmp    %ebx,%esi
801025af:	72 23                	jb     801025d4 <kinit2+0x44>
801025b1:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    kfree(p);
801025b8:	83 ec 0c             	sub    $0xc,%esp
801025bb:	8d 83 00 f0 ff ff    	lea    -0x1000(%ebx),%eax
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
801025c1:	81 c3 00 10 00 00    	add    $0x1000,%ebx
    kfree(p);
801025c7:	50                   	push   %eax
801025c8:	e8 d3 fe ff ff       	call   801024a0 <kfree>
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
801025cd:	83 c4 10             	add    $0x10,%esp
801025d0:	39 de                	cmp    %ebx,%esi
801025d2:	73 e4                	jae    801025b8 <kinit2+0x28>
  kmem.use_lock = 1;
801025d4:	c7 05 74 16 11 80 01 	movl   $0x1,0x80111674
801025db:	00 00 00 
}
801025de:	8d 65 f8             	lea    -0x8(%ebp),%esp
801025e1:	5b                   	pop    %ebx
801025e2:	5e                   	pop    %esi
801025e3:	5d                   	pop    %ebp
801025e4:	c3                   	ret
801025e5:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801025ec:	00 
801025ed:	8d 76 00             	lea    0x0(%esi),%esi

801025f0 <kinit1>:
{
801025f0:	55                   	push   %ebp
801025f1:	89 e5                	mov    %esp,%ebp
801025f3:	56                   	push   %esi
801025f4:	53                   	push   %ebx
801025f5:	8b 75 0c             	mov    0xc(%ebp),%esi
  initlock(&kmem.lock, "kmem");
801025f8:	83 ec 08             	sub    $0x8,%esp
801025fb:	68 32 73 10 80       	push   $0x80107332
80102600:	68 40 16 11 80       	push   $0x80111640
80102605:	e8 a6 1d 00 00       	call   801043b0 <initlock>
  p = (char*)PGROUNDUP((uint)vstart);
8010260a:	8b 45 08             	mov    0x8(%ebp),%eax
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
8010260d:	83 c4 10             	add    $0x10,%esp
  kmem.use_lock = 0;
80102610:	c7 05 74 16 11 80 00 	movl   $0x0,0x80111674
80102617:	00 00 00 
  p = (char*)PGROUNDUP((uint)vstart);
8010261a:	8d 98 ff 0f 00 00    	lea    0xfff(%eax),%ebx
80102620:	81 e3 00 f0 ff ff    	and    $0xfffff000,%ebx
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
80102626:	81 c3 00 10 00 00    	add    $0x1000,%ebx
8010262c:	39 de                	cmp    %ebx,%esi
8010262e:	72 1c                	jb     8010264c <kinit1+0x5c>
    kfree(p);
80102630:	83 ec 0c             	sub    $0xc,%esp
80102633:	8d 83 00 f0 ff ff    	lea    -0x1000(%ebx),%eax
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
80102639:	81 c3 00 10 00 00    	add    $0x1000,%ebx
    kfree(p);
8010263f:	50                   	push   %eax
80102640:	e8 5b fe ff ff       	call   801024a0 <kfree>
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
80102645:	83 c4 10             	add    $0x10,%esp
80102648:	39 de                	cmp    %ebx,%esi
8010264a:	73 e4                	jae    80102630 <kinit1+0x40>
}
8010264c:	8d 65 f8             	lea    -0x8(%ebp),%esp
8010264f:	5b                   	pop    %ebx
80102650:	5e                   	pop    %esi
80102651:	5d                   	pop    %ebp
80102652:	c3                   	ret
80102653:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010265a:	00 
8010265b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi

80102660 <kalloc>:
// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
80102660:	55                   	push   %ebp
80102661:	89 e5                	mov    %esp,%ebp
80102663:	53                   	push   %ebx
80102664:	83 ec 04             	sub    $0x4,%esp
  struct run *r;

  if(kmem.use_lock)
80102667:	a1 74 16 11 80       	mov    0x80111674,%eax
8010266c:	85 c0                	test   %eax,%eax
8010266e:	75 20                	jne    80102690 <kalloc+0x30>
    acquire(&kmem.lock);
  r = kmem.freelist;
80102670:	8b 1d 78 16 11 80    	mov    0x80111678,%ebx
  if(r)
80102676:	85 db                	test   %ebx,%ebx
80102678:	74 07                	je     80102681 <kalloc+0x21>
    kmem.freelist = r->next;
8010267a:	8b 03                	mov    (%ebx),%eax
8010267c:	a3 78 16 11 80       	mov    %eax,0x80111678
  if(kmem.use_lock)
    release(&kmem.lock);
  return (char*)r;
}
80102681:	89 d8                	mov    %ebx,%eax
80102683:	8b 5d fc             	mov    -0x4(%ebp),%ebx
80102686:	c9                   	leave
80102687:	c3                   	ret
80102688:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010268f:	00 
    acquire(&kmem.lock);
80102690:	83 ec 0c             	sub    $0xc,%esp
80102693:	68 40 16 11 80       	push   $0x80111640
80102698:	e8 03 1f 00 00       	call   801045a0 <acquire>
  r = kmem.freelist;
8010269d:	8b 1d 78 16 11 80    	mov    0x80111678,%ebx
  if(kmem.use_lock)
801026a3:	a1 74 16 11 80       	mov    0x80111674,%eax
  if(r)
801026a8:	83 c4 10             	add    $0x10,%esp
801026ab:	85 db                	test   %ebx,%ebx
801026ad:	74 08                	je     801026b7 <kalloc+0x57>
    kmem.freelist = r->next;
801026af:	8b 13                	mov    (%ebx),%edx
801026b1:	89 15 78 16 11 80    	mov    %edx,0x80111678
  if(kmem.use_lock)
801026b7:	85 c0                	test   %eax,%eax
801026b9:	74 c6                	je     80102681 <kalloc+0x21>
    release(&kmem.lock);
801026bb:	83 ec 0c             	sub    $0xc,%esp
801026be:	68 40 16 11 80       	push   $0x80111640
801026c3:	e8 78 1e 00 00       	call   80104540 <release>
}
801026c8:	89 d8                	mov    %ebx,%eax
    release(&kmem.lock);
801026ca:	83 c4 10             	add    $0x10,%esp
}
801026cd:	8b 5d fc             	mov    -0x4(%ebp),%ebx
801026d0:	c9                   	leave
801026d1:	c3                   	ret
801026d2:	66 90                	xchg   %ax,%ax
801026d4:	66 90                	xchg   %ax,%ax
801026d6:	66 90                	xchg   %ax,%ax
801026d8:	66 90                	xchg   %ax,%ax
801026da:	66 90                	xchg   %ax,%ax
801026dc:	66 90                	xchg   %ax,%ax
801026de:	66 90                	xchg   %ax,%ax

801026e0 <kbdgetc>:
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
801026e0:	ba 64 00 00 00       	mov    $0x64,%edx
801026e5:	ec                   	in     (%dx),%al
    normalmap, shiftmap, ctlmap, ctlmap
  };
  uint st, data, c;

  st = inb(KBSTATP);
  if((st & KBS_DIB) == 0)
801026e6:	a8 01                	test   $0x1,%al
801026e8:	0f 84 c2 00 00 00    	je     801027b0 <kbdgetc+0xd0>
{
801026ee:	55                   	push   %ebp
801026ef:	ba 60 00 00 00       	mov    $0x60,%edx
801026f4:	89 e5                	mov    %esp,%ebp
801026f6:	53                   	push   %ebx
801026f7:	ec                   	in     (%dx),%al
    return -1;
  data = inb(KBDATAP);

  if(data == 0xE0){
    shift |= E0ESC;
801026f8:	8b 1d 7c 16 11 80    	mov    0x8011167c,%ebx
  data = inb(KBDATAP);
801026fe:	0f b6 c8             	movzbl %al,%ecx
  if(data == 0xE0){
80102701:	3c e0                	cmp    $0xe0,%al
80102703:	74 5b                	je     80102760 <kbdgetc+0x80>
    return 0;
  } else if(data & 0x80){
    // Key released
    data = (shift & E0ESC ? data : data & 0x7F);
80102705:	89 da                	mov    %ebx,%edx
80102707:	83 e2 40             	and    $0x40,%edx
  } else if(data & 0x80){
8010270a:	84 c0                	test   %al,%al
8010270c:	78 62                	js     80102770 <kbdgetc+0x90>
    shift &= ~(shiftcode[data] | E0ESC);
    return 0;
  } else if(shift & E0ESC){
8010270e:	85 d2                	test   %edx,%edx
80102710:	74 09                	je     8010271b <kbdgetc+0x3b>
    // Last character was an E0 escape; or with 0x80
    data |= 0x80;
80102712:	83 c8 80             	or     $0xffffff80,%eax
    shift &= ~E0ESC;
80102715:	83 e3 bf             	and    $0xffffffbf,%ebx
    data |= 0x80;
80102718:	0f b6 c8             	movzbl %al,%ecx
  }

  shift |= shiftcode[data];
8010271b:	0f b6 91 40 79 10 80 	movzbl -0x7fef86c0(%ecx),%edx
  shift ^= togglecode[data];
80102722:	0f b6 81 40 78 10 80 	movzbl -0x7fef87c0(%ecx),%eax
  shift |= shiftcode[data];
80102729:	09 da                	or     %ebx,%edx
  shift ^= togglecode[data];
8010272b:	31 c2                	xor    %eax,%edx
  c = charcode[shift & (CTL | SHIFT)][data];
8010272d:	89 d0                	mov    %edx,%eax
  shift ^= togglecode[data];
8010272f:	89 15 7c 16 11 80    	mov    %edx,0x8011167c
  c = charcode[shift & (CTL | SHIFT)][data];
80102735:	83 e0 03             	and    $0x3,%eax
  if(shift & CAPSLOCK){
80102738:	83 e2 08             	and    $0x8,%edx
  c = charcode[shift & (CTL | SHIFT)][data];
8010273b:	8b 04 85 20 78 10 80 	mov    -0x7fef87e0(,%eax,4),%eax
80102742:	0f b6 04 08          	movzbl (%eax,%ecx,1),%eax
  if(shift & CAPSLOCK){
80102746:	74 0b                	je     80102753 <kbdgetc+0x73>
    if('a' <= c && c <= 'z')
80102748:	8d 50 9f             	lea    -0x61(%eax),%edx
8010274b:	83 fa 19             	cmp    $0x19,%edx
8010274e:	77 48                	ja     80102798 <kbdgetc+0xb8>
      c += 'A' - 'a';
80102750:	83 e8 20             	sub    $0x20,%eax
    else if('A' <= c && c <= 'Z')
      c += 'a' - 'A';
  }
  return c;
}
80102753:	8b 5d fc             	mov    -0x4(%ebp),%ebx
80102756:	c9                   	leave
80102757:	c3                   	ret
80102758:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010275f:	00 
    shift |= E0ESC;
80102760:	83 cb 40             	or     $0x40,%ebx
    return 0;
80102763:	31 c0                	xor    %eax,%eax
    shift |= E0ESC;
80102765:	89 1d 7c 16 11 80    	mov    %ebx,0x8011167c
}
8010276b:	8b 5d fc             	mov    -0x4(%ebp),%ebx
8010276e:	c9                   	leave
8010276f:	c3                   	ret
    data = (shift & E0ESC ? data : data & 0x7F);
80102770:	83 e0 7f             	and    $0x7f,%eax
80102773:	85 d2                	test   %edx,%edx
80102775:	0f 44 c8             	cmove  %eax,%ecx
    shift &= ~(shiftcode[data] | E0ESC);
80102778:	0f b6 81 40 79 10 80 	movzbl -0x7fef86c0(%ecx),%eax
8010277f:	83 c8 40             	or     $0x40,%eax
80102782:	0f b6 c0             	movzbl %al,%eax
80102785:	f7 d0                	not    %eax
80102787:	21 d8                	and    %ebx,%eax
80102789:	a3 7c 16 11 80       	mov    %eax,0x8011167c
    return 0;
8010278e:	31 c0                	xor    %eax,%eax
80102790:	eb d9                	jmp    8010276b <kbdgetc+0x8b>
80102792:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    else if('A' <= c && c <= 'Z')
80102798:	8d 48 bf             	lea    -0x41(%eax),%ecx
      c += 'a' - 'A';
8010279b:	8d 50 20             	lea    0x20(%eax),%edx
}
8010279e:	8b 5d fc             	mov    -0x4(%ebp),%ebx
801027a1:	c9                   	leave
      c += 'a' - 'A';
801027a2:	83 f9 1a             	cmp    $0x1a,%ecx
801027a5:	0f 42 c2             	cmovb  %edx,%eax
}
801027a8:	c3                   	ret
801027a9:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    return -1;
801027b0:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
}
801027b5:	c3                   	ret
801027b6:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801027bd:	00 
801027be:	66 90                	xchg   %ax,%ax

801027c0 <kbdintr>:

void
kbdintr(void)
{
801027c0:	55                   	push   %ebp
801027c1:	89 e5                	mov    %esp,%ebp
801027c3:	83 ec 14             	sub    $0x14,%esp
  consoleintr(kbdgetc);
801027c6:	68 e0 26 10 80       	push   $0x801026e0
801027cb:	e8 d0 e0 ff ff       	call   801008a0 <consoleintr>
}
801027d0:	83 c4 10             	add    $0x10,%esp
801027d3:	c9                   	leave
801027d4:	c3                   	ret
801027d5:	66 90                	xchg   %ax,%ax
801027d7:	66 90                	xchg   %ax,%ax
801027d9:	66 90                	xchg   %ax,%ax
801027db:	66 90                	xchg   %ax,%ax
801027dd:	66 90                	xchg   %ax,%ax
801027df:	90                   	nop

801027e0 <lapicinit>:
}

void
lapicinit(void)
{
  if(!lapic)
801027e0:	a1 80 16 11 80       	mov    0x80111680,%eax
801027e5:	85 c0                	test   %eax,%eax
801027e7:	0f 84 c3 00 00 00    	je     801028b0 <lapicinit+0xd0>
  lapic[index] = value;
801027ed:	c7 80 f0 00 00 00 3f 	movl   $0x13f,0xf0(%eax)
801027f4:	01 00 00 
  lapic[ID];  // wait for write to finish, by reading
801027f7:	8b 50 20             	mov    0x20(%eax),%edx
  lapic[index] = value;
801027fa:	c7 80 e0 03 00 00 0b 	movl   $0xb,0x3e0(%eax)
80102801:	00 00 00 
  lapic[ID];  // wait for write to finish, by reading
80102804:	8b 50 20             	mov    0x20(%eax),%edx
  lapic[index] = value;
80102807:	c7 80 20 03 00 00 20 	movl   $0x20020,0x320(%eax)
8010280e:	00 02 00 
  lapic[ID];  // wait for write to finish, by reading
80102811:	8b 50 20             	mov    0x20(%eax),%edx
  lapic[index] = value;
80102814:	c7 80 80 03 00 00 80 	movl   $0x989680,0x380(%eax)
8010281b:	96 98 00 
  lapic[ID];  // wait for write to finish, by reading
8010281e:	8b 50 20             	mov    0x20(%eax),%edx
  lapic[index] = value;
80102821:	c7 80 50 03 00 00 00 	movl   $0x10000,0x350(%eax)
80102828:	00 01 00 
  lapic[ID];  // wait for write to finish, by reading
8010282b:	8b 50 20             	mov    0x20(%eax),%edx
  lapic[index] = value;
8010282e:	c7 80 60 03 00 00 00 	movl   $0x10000,0x360(%eax)
80102835:	00 01 00 
  lapic[ID];  // wait for write to finish, by reading
80102838:	8b 50 20             	mov    0x20(%eax),%edx
  lapicw(LINT0, MASKED);
  lapicw(LINT1, MASKED);

  // Disable performance counter overflow interrupts
  // on machines that provide that interrupt entry.
  if(((lapic[VER]>>16) & 0xFF) >= 4)
8010283b:	8b 50 30             	mov    0x30(%eax),%edx
8010283e:	81 e2 00 00 fc 00    	and    $0xfc0000,%edx
80102844:	75 72                	jne    801028b8 <lapicinit+0xd8>
  lapic[index] = value;
80102846:	c7 80 70 03 00 00 33 	movl   $0x33,0x370(%eax)
8010284d:	00 00 00 
  lapic[ID];  // wait for write to finish, by reading
80102850:	8b 50 20             	mov    0x20(%eax),%edx
  lapic[index] = value;
80102853:	c7 80 80 02 00 00 00 	movl   $0x0,0x280(%eax)
8010285a:	00 00 00 
  lapic[ID];  // wait for write to finish, by reading
8010285d:	8b 50 20             	mov    0x20(%eax),%edx
  lapic[index] = value;
80102860:	c7 80 80 02 00 00 00 	movl   $0x0,0x280(%eax)
80102867:	00 00 00 
  lapic[ID];  // wait for write to finish, by reading
8010286a:	8b 50 20             	mov    0x20(%eax),%edx
  lapic[index] = value;
8010286d:	c7 80 b0 00 00 00 00 	movl   $0x0,0xb0(%eax)
80102874:	00 00 00 
  lapic[ID];  // wait for write to finish, by reading
80102877:	8b 50 20             	mov    0x20(%eax),%edx
  lapic[index] = value;
8010287a:	c7 80 10 03 00 00 00 	movl   $0x0,0x310(%eax)
80102881:	00 00 00 
  lapic[ID];  // wait for write to finish, by reading
80102884:	8b 50 20             	mov    0x20(%eax),%edx
  lapic[index] = value;
80102887:	c7 80 00 03 00 00 00 	movl   $0x88500,0x300(%eax)
8010288e:	85 08 00 
  lapic[ID];  // wait for write to finish, by reading
80102891:	8b 50 20             	mov    0x20(%eax),%edx
  lapicw(EOI, 0);

  // Send an Init Level De-Assert to synchronise arbitration ID's.
  lapicw(ICRHI, 0);
  lapicw(ICRLO, BCAST | INIT | LEVEL);
  while(lapic[ICRLO] & DELIVS)
80102894:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
80102898:	8b 90 00 03 00 00    	mov    0x300(%eax),%edx
8010289e:	80 e6 10             	and    $0x10,%dh
801028a1:	75 f5                	jne    80102898 <lapicinit+0xb8>
  lapic[index] = value;
801028a3:	c7 80 80 00 00 00 00 	movl   $0x0,0x80(%eax)
801028aa:	00 00 00 
  lapic[ID];  // wait for write to finish, by reading
801028ad:	8b 40 20             	mov    0x20(%eax),%eax
    ;

  // Enable interrupts on the APIC (but not on the processor).
  lapicw(TPR, 0);
}
801028b0:	c3                   	ret
801028b1:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
  lapic[index] = value;
801028b8:	c7 80 40 03 00 00 00 	movl   $0x10000,0x340(%eax)
801028bf:	00 01 00 
  lapic[ID];  // wait for write to finish, by reading
801028c2:	8b 50 20             	mov    0x20(%eax),%edx
}
801028c5:	e9 7c ff ff ff       	jmp    80102846 <lapicinit+0x66>
801028ca:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi

801028d0 <lapicid>:

int
lapicid(void)
{
  if (!lapic)
801028d0:	a1 80 16 11 80       	mov    0x80111680,%eax
801028d5:	85 c0                	test   %eax,%eax
801028d7:	74 07                	je     801028e0 <lapicid+0x10>
    return 0;
  return lapic[ID] >> 24;
801028d9:	8b 40 20             	mov    0x20(%eax),%eax
801028dc:	c1 e8 18             	shr    $0x18,%eax
801028df:	c3                   	ret
    return 0;
801028e0:	31 c0                	xor    %eax,%eax
}
801028e2:	c3                   	ret
801028e3:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801028ea:	00 
801028eb:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi

801028f0 <lapiceoi>:

// Acknowledge interrupt.
void
lapiceoi(void)
{
  if(lapic)
801028f0:	a1 80 16 11 80       	mov    0x80111680,%eax
801028f5:	85 c0                	test   %eax,%eax
801028f7:	74 0d                	je     80102906 <lapiceoi+0x16>
  lapic[index] = value;
801028f9:	c7 80 b0 00 00 00 00 	movl   $0x0,0xb0(%eax)
80102900:	00 00 00 
  lapic[ID];  // wait for write to finish, by reading
80102903:	8b 40 20             	mov    0x20(%eax),%eax
    lapicw(EOI, 0);
}
80102906:	c3                   	ret
80102907:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010290e:	00 
8010290f:	90                   	nop

80102910 <microdelay>:
// Spin for a given number of microseconds.
// On real hardware would want to tune this dynamically.
void
microdelay(int us)
{
}
80102910:	c3                   	ret
80102911:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80102918:	00 
80102919:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi

80102920 <lapicstartap>:

// Start additional processor running entry code at addr.
// See Appendix B of MultiProcessor Specification.
void
lapicstartap(uchar apicid, uint addr)
{
80102920:	55                   	push   %ebp
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
80102921:	b8 0f 00 00 00       	mov    $0xf,%eax
80102926:	ba 70 00 00 00       	mov    $0x70,%edx
8010292b:	89 e5                	mov    %esp,%ebp
8010292d:	53                   	push   %ebx
8010292e:	8b 4d 0c             	mov    0xc(%ebp),%ecx
80102931:	8b 5d 08             	mov    0x8(%ebp),%ebx
80102934:	ee                   	out    %al,(%dx)
80102935:	b8 0a 00 00 00       	mov    $0xa,%eax
8010293a:	ba 71 00 00 00       	mov    $0x71,%edx
8010293f:	ee                   	out    %al,(%dx)
  // and the warm reset vector (DWORD based at 40:67) to point at
  // the AP startup code prior to the [universal startup algorithm]."
  outb(CMOS_PORT, 0xF);  // offset 0xF is shutdown code
  outb(CMOS_PORT+1, 0x0A);
  wrv = (ushort*)P2V((0x40<<4 | 0x67));  // Warm reset vector
  wrv[0] = 0;
80102940:	31 c0                	xor    %eax,%eax
  lapic[index] = value;
80102942:	c1 e3 18             	shl    $0x18,%ebx
  wrv[0] = 0;
80102945:	66 a3 67 04 00 80    	mov    %ax,0x80000467
  wrv[1] = addr >> 4;
8010294b:	89 c8                	mov    %ecx,%eax
  // when it is in the halted state due to an INIT.  So the second
  // should be ignored, but it is part of the official Intel algorithm.
  // Bochs complains about the second one.  Too bad for Bochs.
  for(i = 0; i < 2; i++){
    lapicw(ICRHI, apicid<<24);
    lapicw(ICRLO, STARTUP | (addr>>12));
8010294d:	c1 e9 0c             	shr    $0xc,%ecx
  lapic[index] = value;
80102950:	89 da                	mov    %ebx,%edx
  wrv[1] = addr >> 4;
80102952:	c1 e8 04             	shr    $0x4,%eax
    lapicw(ICRLO, STARTUP | (addr>>12));
80102955:	80 cd 06             	or     $0x6,%ch
  wrv[1] = addr >> 4;
80102958:	66 a3 69 04 00 80    	mov    %ax,0x80000469
  lapic[index] = value;
8010295e:	a1 80 16 11 80       	mov    0x80111680,%eax
80102963:	89 98 10 03 00 00    	mov    %ebx,0x310(%eax)
  lapic[ID];  // wait for write to finish, by reading
80102969:	8b 58 20             	mov    0x20(%eax),%ebx
  lapic[index] = value;
8010296c:	c7 80 00 03 00 00 00 	movl   $0xc500,0x300(%eax)
80102973:	c5 00 00 
  lapic[ID];  // wait for write to finish, by reading
80102976:	8b 58 20             	mov    0x20(%eax),%ebx
  lapic[index] = value;
80102979:	c7 80 00 03 00 00 00 	movl   $0x8500,0x300(%eax)
80102980:	85 00 00 
  lapic[ID];  // wait for write to finish, by reading
80102983:	8b 58 20             	mov    0x20(%eax),%ebx
  lapic[index] = value;
80102986:	89 90 10 03 00 00    	mov    %edx,0x310(%eax)
  lapic[ID];  // wait for write to finish, by reading
8010298c:	8b 58 20             	mov    0x20(%eax),%ebx
  lapic[index] = value;
8010298f:	89 88 00 03 00 00    	mov    %ecx,0x300(%eax)
  lapic[ID];  // wait for write to finish, by reading
80102995:	8b 58 20             	mov    0x20(%eax),%ebx
  lapic[index] = value;
80102998:	89 90 10 03 00 00    	mov    %edx,0x310(%eax)
  lapic[ID];  // wait for write to finish, by reading
8010299e:	8b 50 20             	mov    0x20(%eax),%edx
  lapic[index] = value;
801029a1:	89 88 00 03 00 00    	mov    %ecx,0x300(%eax)
  lapic[ID];  // wait for write to finish, by reading
801029a7:	8b 40 20             	mov    0x20(%eax),%eax
    microdelay(200);
  }
}
801029aa:	8b 5d fc             	mov    -0x4(%ebp),%ebx
801029ad:	c9                   	leave
801029ae:	c3                   	ret
801029af:	90                   	nop

801029b0 <cmostime>:
}

// qemu seems to use 24-hour GWT and the values are BCD encoded
void
cmostime(struct rtcdate *r)
{
801029b0:	55                   	push   %ebp
801029b1:	b8 0b 00 00 00       	mov    $0xb,%eax
801029b6:	ba 70 00 00 00       	mov    $0x70,%edx
801029bb:	89 e5                	mov    %esp,%ebp
801029bd:	57                   	push   %edi
801029be:	56                   	push   %esi
801029bf:	53                   	push   %ebx
801029c0:	83 ec 4c             	sub    $0x4c,%esp
801029c3:	ee                   	out    %al,(%dx)
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
801029c4:	ba 71 00 00 00       	mov    $0x71,%edx
801029c9:	ec                   	in     (%dx),%al
  struct rtcdate t1, t2;
  int sb, bcd;

  sb = cmos_read(CMOS_STATB);

  bcd = (sb & (1 << 2)) == 0;
801029ca:	83 e0 04             	and    $0x4,%eax
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
801029cd:	bf 70 00 00 00       	mov    $0x70,%edi
801029d2:	88 45 b3             	mov    %al,-0x4d(%ebp)
801029d5:	8d 76 00             	lea    0x0(%esi),%esi
801029d8:	31 c0                	xor    %eax,%eax
801029da:	89 fa                	mov    %edi,%edx
801029dc:	ee                   	out    %al,(%dx)
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
801029dd:	b9 71 00 00 00       	mov    $0x71,%ecx
801029e2:	89 ca                	mov    %ecx,%edx
801029e4:	ec                   	in     (%dx),%al
801029e5:	88 45 b7             	mov    %al,-0x49(%ebp)
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
801029e8:	89 fa                	mov    %edi,%edx
801029ea:	b8 02 00 00 00       	mov    $0x2,%eax
801029ef:	ee                   	out    %al,(%dx)
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
801029f0:	89 ca                	mov    %ecx,%edx
801029f2:	ec                   	in     (%dx),%al
801029f3:	88 45 b6             	mov    %al,-0x4a(%ebp)
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
801029f6:	89 fa                	mov    %edi,%edx
801029f8:	b8 04 00 00 00       	mov    $0x4,%eax
801029fd:	ee                   	out    %al,(%dx)
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
801029fe:	89 ca                	mov    %ecx,%edx
80102a00:	ec                   	in     (%dx),%al
80102a01:	88 45 b5             	mov    %al,-0x4b(%ebp)
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
80102a04:	89 fa                	mov    %edi,%edx
80102a06:	b8 07 00 00 00       	mov    $0x7,%eax
80102a0b:	ee                   	out    %al,(%dx)
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
80102a0c:	89 ca                	mov    %ecx,%edx
80102a0e:	ec                   	in     (%dx),%al
80102a0f:	88 45 b4             	mov    %al,-0x4c(%ebp)
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
80102a12:	89 fa                	mov    %edi,%edx
80102a14:	b8 08 00 00 00       	mov    $0x8,%eax
80102a19:	ee                   	out    %al,(%dx)
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
80102a1a:	89 ca                	mov    %ecx,%edx
80102a1c:	ec                   	in     (%dx),%al
80102a1d:	89 c6                	mov    %eax,%esi
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
80102a1f:	89 fa                	mov    %edi,%edx
80102a21:	b8 09 00 00 00       	mov    $0x9,%eax
80102a26:	ee                   	out    %al,(%dx)
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
80102a27:	89 ca                	mov    %ecx,%edx
80102a29:	ec                   	in     (%dx),%al
80102a2a:	0f b6 d8             	movzbl %al,%ebx
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
80102a2d:	89 fa                	mov    %edi,%edx
80102a2f:	b8 0a 00 00 00       	mov    $0xa,%eax
80102a34:	ee                   	out    %al,(%dx)
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
80102a35:	89 ca                	mov    %ecx,%edx
80102a37:	ec                   	in     (%dx),%al

  // make sure CMOS doesn't modify time while we read it
  for(;;) {
    fill_rtcdate(&t1);
    if(cmos_read(CMOS_STATA) & CMOS_UIP)
80102a38:	84 c0                	test   %al,%al
80102a3a:	78 9c                	js     801029d8 <cmostime+0x28>
  return inb(CMOS_RETURN);
80102a3c:	0f b6 45 b7          	movzbl -0x49(%ebp),%eax
80102a40:	89 f2                	mov    %esi,%edx
80102a42:	89 5d cc             	mov    %ebx,-0x34(%ebp)
80102a45:	0f b6 f2             	movzbl %dl,%esi
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
80102a48:	89 fa                	mov    %edi,%edx
80102a4a:	89 45 b8             	mov    %eax,-0x48(%ebp)
80102a4d:	0f b6 45 b6          	movzbl -0x4a(%ebp),%eax
80102a51:	89 75 c8             	mov    %esi,-0x38(%ebp)
80102a54:	89 45 bc             	mov    %eax,-0x44(%ebp)
80102a57:	0f b6 45 b5          	movzbl -0x4b(%ebp),%eax
80102a5b:	89 45 c0             	mov    %eax,-0x40(%ebp)
80102a5e:	0f b6 45 b4          	movzbl -0x4c(%ebp),%eax
80102a62:	89 45 c4             	mov    %eax,-0x3c(%ebp)
80102a65:	31 c0                	xor    %eax,%eax
80102a67:	ee                   	out    %al,(%dx)
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
80102a68:	89 ca                	mov    %ecx,%edx
80102a6a:	ec                   	in     (%dx),%al
80102a6b:	0f b6 c0             	movzbl %al,%eax
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
80102a6e:	89 fa                	mov    %edi,%edx
80102a70:	89 45 d0             	mov    %eax,-0x30(%ebp)
80102a73:	b8 02 00 00 00       	mov    $0x2,%eax
80102a78:	ee                   	out    %al,(%dx)
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
80102a79:	89 ca                	mov    %ecx,%edx
80102a7b:	ec                   	in     (%dx),%al
80102a7c:	0f b6 c0             	movzbl %al,%eax
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
80102a7f:	89 fa                	mov    %edi,%edx
80102a81:	89 45 d4             	mov    %eax,-0x2c(%ebp)
80102a84:	b8 04 00 00 00       	mov    $0x4,%eax
80102a89:	ee                   	out    %al,(%dx)
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
80102a8a:	89 ca                	mov    %ecx,%edx
80102a8c:	ec                   	in     (%dx),%al
80102a8d:	0f b6 c0             	movzbl %al,%eax
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
80102a90:	89 fa                	mov    %edi,%edx
80102a92:	89 45 d8             	mov    %eax,-0x28(%ebp)
80102a95:	b8 07 00 00 00       	mov    $0x7,%eax
80102a9a:	ee                   	out    %al,(%dx)
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
80102a9b:	89 ca                	mov    %ecx,%edx
80102a9d:	ec                   	in     (%dx),%al
80102a9e:	0f b6 c0             	movzbl %al,%eax
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
80102aa1:	89 fa                	mov    %edi,%edx
80102aa3:	89 45 dc             	mov    %eax,-0x24(%ebp)
80102aa6:	b8 08 00 00 00       	mov    $0x8,%eax
80102aab:	ee                   	out    %al,(%dx)
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
80102aac:	89 ca                	mov    %ecx,%edx
80102aae:	ec                   	in     (%dx),%al
80102aaf:	0f b6 c0             	movzbl %al,%eax
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
80102ab2:	89 fa                	mov    %edi,%edx
80102ab4:	89 45 e0             	mov    %eax,-0x20(%ebp)
80102ab7:	b8 09 00 00 00       	mov    $0x9,%eax
80102abc:	ee                   	out    %al,(%dx)
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
80102abd:	89 ca                	mov    %ecx,%edx
80102abf:	ec                   	in     (%dx),%al
80102ac0:	0f b6 c0             	movzbl %al,%eax
        continue;
    fill_rtcdate(&t2);
    if(memcmp(&t1, &t2, sizeof(t1)) == 0)
80102ac3:	83 ec 04             	sub    $0x4,%esp
  return inb(CMOS_RETURN);
80102ac6:	89 45 e4             	mov    %eax,-0x1c(%ebp)
    if(memcmp(&t1, &t2, sizeof(t1)) == 0)
80102ac9:	8d 45 d0             	lea    -0x30(%ebp),%eax
80102acc:	6a 18                	push   $0x18
80102ace:	50                   	push   %eax
80102acf:	8d 45 b8             	lea    -0x48(%ebp),%eax
80102ad2:	50                   	push   %eax
80102ad3:	e8 08 1c 00 00       	call   801046e0 <memcmp>
80102ad8:	83 c4 10             	add    $0x10,%esp
80102adb:	85 c0                	test   %eax,%eax
80102add:	0f 85 f5 fe ff ff    	jne    801029d8 <cmostime+0x28>
      break;
  }

  // convert
  if(bcd) {
80102ae3:	0f b6 75 b3          	movzbl -0x4d(%ebp),%esi
80102ae7:	8b 5d 08             	mov    0x8(%ebp),%ebx
80102aea:	89 f0                	mov    %esi,%eax
80102aec:	84 c0                	test   %al,%al
80102aee:	75 78                	jne    80102b68 <cmostime+0x1b8>
#define    CONV(x)     (t1.x = ((t1.x >> 4) * 10) + (t1.x & 0xf))
    CONV(second);
80102af0:	8b 45 b8             	mov    -0x48(%ebp),%eax
80102af3:	89 c2                	mov    %eax,%edx
80102af5:	83 e0 0f             	and    $0xf,%eax
80102af8:	c1 ea 04             	shr    $0x4,%edx
80102afb:	8d 14 92             	lea    (%edx,%edx,4),%edx
80102afe:	8d 04 50             	lea    (%eax,%edx,2),%eax
80102b01:	89 45 b8             	mov    %eax,-0x48(%ebp)
    CONV(minute);
80102b04:	8b 45 bc             	mov    -0x44(%ebp),%eax
80102b07:	89 c2                	mov    %eax,%edx
80102b09:	83 e0 0f             	and    $0xf,%eax
80102b0c:	c1 ea 04             	shr    $0x4,%edx
80102b0f:	8d 14 92             	lea    (%edx,%edx,4),%edx
80102b12:	8d 04 50             	lea    (%eax,%edx,2),%eax
80102b15:	89 45 bc             	mov    %eax,-0x44(%ebp)
    CONV(hour  );
80102b18:	8b 45 c0             	mov    -0x40(%ebp),%eax
80102b1b:	89 c2                	mov    %eax,%edx
80102b1d:	83 e0 0f             	and    $0xf,%eax
80102b20:	c1 ea 04             	shr    $0x4,%edx
80102b23:	8d 14 92             	lea    (%edx,%edx,4),%edx
80102b26:	8d 04 50             	lea    (%eax,%edx,2),%eax
80102b29:	89 45 c0             	mov    %eax,-0x40(%ebp)
    CONV(day   );
80102b2c:	8b 45 c4             	mov    -0x3c(%ebp),%eax
80102b2f:	89 c2                	mov    %eax,%edx
80102b31:	83 e0 0f             	and    $0xf,%eax
80102b34:	c1 ea 04             	shr    $0x4,%edx
80102b37:	8d 14 92             	lea    (%edx,%edx,4),%edx
80102b3a:	8d 04 50             	lea    (%eax,%edx,2),%eax
80102b3d:	89 45 c4             	mov    %eax,-0x3c(%ebp)
    CONV(month );
80102b40:	8b 45 c8             	mov    -0x38(%ebp),%eax
80102b43:	89 c2                	mov    %eax,%edx
80102b45:	83 e0 0f             	and    $0xf,%eax
80102b48:	c1 ea 04             	shr    $0x4,%edx
80102b4b:	8d 14 92             	lea    (%edx,%edx,4),%edx
80102b4e:	8d 04 50             	lea    (%eax,%edx,2),%eax
80102b51:	89 45 c8             	mov    %eax,-0x38(%ebp)
    CONV(year  );
80102b54:	8b 45 cc             	mov    -0x34(%ebp),%eax
80102b57:	89 c2                	mov    %eax,%edx
80102b59:	83 e0 0f             	and    $0xf,%eax
80102b5c:	c1 ea 04             	shr    $0x4,%edx
80102b5f:	8d 14 92             	lea    (%edx,%edx,4),%edx
80102b62:	8d 04 50             	lea    (%eax,%edx,2),%eax
80102b65:	89 45 cc             	mov    %eax,-0x34(%ebp)
#undef     CONV
  }

  *r = t1;
80102b68:	8b 45 b8             	mov    -0x48(%ebp),%eax
80102b6b:	89 03                	mov    %eax,(%ebx)
80102b6d:	8b 45 bc             	mov    -0x44(%ebp),%eax
80102b70:	89 43 04             	mov    %eax,0x4(%ebx)
80102b73:	8b 45 c0             	mov    -0x40(%ebp),%eax
80102b76:	89 43 08             	mov    %eax,0x8(%ebx)
80102b79:	8b 45 c4             	mov    -0x3c(%ebp),%eax
80102b7c:	89 43 0c             	mov    %eax,0xc(%ebx)
80102b7f:	8b 45 c8             	mov    -0x38(%ebp),%eax
80102b82:	89 43 10             	mov    %eax,0x10(%ebx)
80102b85:	8b 45 cc             	mov    -0x34(%ebp),%eax
80102b88:	89 43 14             	mov    %eax,0x14(%ebx)
  r->year += 2000;
80102b8b:	81 43 14 d0 07 00 00 	addl   $0x7d0,0x14(%ebx)
}
80102b92:	8d 65 f4             	lea    -0xc(%ebp),%esp
80102b95:	5b                   	pop    %ebx
80102b96:	5e                   	pop    %esi
80102b97:	5f                   	pop    %edi
80102b98:	5d                   	pop    %ebp
80102b99:	c3                   	ret
80102b9a:	66 90                	xchg   %ax,%ax
80102b9c:	66 90                	xchg   %ax,%ax
80102b9e:	66 90                	xchg   %ax,%ax

80102ba0 <install_trans>:
static void
install_trans(void)
{
  int tail;

  for (tail = 0; tail < log.lh.n; tail++) {
80102ba0:	8b 0d e8 16 11 80    	mov    0x801116e8,%ecx
80102ba6:	85 c9                	test   %ecx,%ecx
80102ba8:	0f 8e 8a 00 00 00    	jle    80102c38 <install_trans+0x98>
{
80102bae:	55                   	push   %ebp
80102baf:	89 e5                	mov    %esp,%ebp
80102bb1:	57                   	push   %edi
  for (tail = 0; tail < log.lh.n; tail++) {
80102bb2:	31 ff                	xor    %edi,%edi
{
80102bb4:	56                   	push   %esi
80102bb5:	53                   	push   %ebx
80102bb6:	83 ec 0c             	sub    $0xc,%esp
80102bb9:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    struct buf *lbuf = bread(log.dev, log.start+tail+1); // read log block
80102bc0:	a1 d4 16 11 80       	mov    0x801116d4,%eax
80102bc5:	83 ec 08             	sub    $0x8,%esp
80102bc8:	01 f8                	add    %edi,%eax
80102bca:	83 c0 01             	add    $0x1,%eax
80102bcd:	50                   	push   %eax
80102bce:	ff 35 e4 16 11 80    	push   0x801116e4
80102bd4:	e8 f7 d4 ff ff       	call   801000d0 <bread>
80102bd9:	89 c6                	mov    %eax,%esi
    struct buf *dbuf = bread(log.dev, log.lh.block[tail]); // read dst
80102bdb:	58                   	pop    %eax
80102bdc:	5a                   	pop    %edx
80102bdd:	ff 34 bd ec 16 11 80 	push   -0x7feee914(,%edi,4)
80102be4:	ff 35 e4 16 11 80    	push   0x801116e4
  for (tail = 0; tail < log.lh.n; tail++) {
80102bea:	83 c7 01             	add    $0x1,%edi
    struct buf *dbuf = bread(log.dev, log.lh.block[tail]); // read dst
80102bed:	e8 de d4 ff ff       	call   801000d0 <bread>
    memmove(dbuf->data, lbuf->data, BSIZE);  // copy block to dst
80102bf2:	83 c4 0c             	add    $0xc,%esp
    struct buf *dbuf = bread(log.dev, log.lh.block[tail]); // read dst
80102bf5:	89 c3                	mov    %eax,%ebx
    memmove(dbuf->data, lbuf->data, BSIZE);  // copy block to dst
80102bf7:	8d 46 5c             	lea    0x5c(%esi),%eax
80102bfa:	68 00 02 00 00       	push   $0x200
80102bff:	50                   	push   %eax
80102c00:	8d 43 5c             	lea    0x5c(%ebx),%eax
80102c03:	50                   	push   %eax
80102c04:	e8 27 1b 00 00       	call   80104730 <memmove>
    bwrite(dbuf);  // write dst to disk
80102c09:	89 1c 24             	mov    %ebx,(%esp)
80102c0c:	e8 9f d5 ff ff       	call   801001b0 <bwrite>
    brelse(lbuf);
80102c11:	89 34 24             	mov    %esi,(%esp)
80102c14:	e8 d7 d5 ff ff       	call   801001f0 <brelse>
    brelse(dbuf);
80102c19:	89 1c 24             	mov    %ebx,(%esp)
80102c1c:	e8 cf d5 ff ff       	call   801001f0 <brelse>
  for (tail = 0; tail < log.lh.n; tail++) {
80102c21:	83 c4 10             	add    $0x10,%esp
80102c24:	39 3d e8 16 11 80    	cmp    %edi,0x801116e8
80102c2a:	7f 94                	jg     80102bc0 <install_trans+0x20>
  }
}
80102c2c:	8d 65 f4             	lea    -0xc(%ebp),%esp
80102c2f:	5b                   	pop    %ebx
80102c30:	5e                   	pop    %esi
80102c31:	5f                   	pop    %edi
80102c32:	5d                   	pop    %ebp
80102c33:	c3                   	ret
80102c34:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
80102c38:	c3                   	ret
80102c39:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi

80102c40 <write_head>:
// Write in-memory log header to disk.
// This is the true point at which the
// current transaction commits.
static void
write_head(void)
{
80102c40:	55                   	push   %ebp
80102c41:	89 e5                	mov    %esp,%ebp
80102c43:	53                   	push   %ebx
80102c44:	83 ec 0c             	sub    $0xc,%esp
  struct buf *buf = bread(log.dev, log.start);
80102c47:	ff 35 d4 16 11 80    	push   0x801116d4
80102c4d:	ff 35 e4 16 11 80    	push   0x801116e4
80102c53:	e8 78 d4 ff ff       	call   801000d0 <bread>
  struct logheader *hb = (struct logheader *) (buf->data);
  int i;
  hb->n = log.lh.n;
  for (i = 0; i < log.lh.n; i++) {
80102c58:	83 c4 10             	add    $0x10,%esp
  struct buf *buf = bread(log.dev, log.start);
80102c5b:	89 c3                	mov    %eax,%ebx
  hb->n = log.lh.n;
80102c5d:	a1 e8 16 11 80       	mov    0x801116e8,%eax
80102c62:	89 43 5c             	mov    %eax,0x5c(%ebx)
  for (i = 0; i < log.lh.n; i++) {
80102c65:	85 c0                	test   %eax,%eax
80102c67:	7e 19                	jle    80102c82 <write_head+0x42>
80102c69:	31 d2                	xor    %edx,%edx
80102c6b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
    hb->block[i] = log.lh.block[i];
80102c70:	8b 0c 95 ec 16 11 80 	mov    -0x7feee914(,%edx,4),%ecx
80102c77:	89 4c 93 60          	mov    %ecx,0x60(%ebx,%edx,4)
  for (i = 0; i < log.lh.n; i++) {
80102c7b:	83 c2 01             	add    $0x1,%edx
80102c7e:	39 d0                	cmp    %edx,%eax
80102c80:	75 ee                	jne    80102c70 <write_head+0x30>
  }
  bwrite(buf);
80102c82:	83 ec 0c             	sub    $0xc,%esp
80102c85:	53                   	push   %ebx
80102c86:	e8 25 d5 ff ff       	call   801001b0 <bwrite>
  brelse(buf);
80102c8b:	89 1c 24             	mov    %ebx,(%esp)
80102c8e:	e8 5d d5 ff ff       	call   801001f0 <brelse>
}
80102c93:	8b 5d fc             	mov    -0x4(%ebp),%ebx
80102c96:	83 c4 10             	add    $0x10,%esp
80102c99:	c9                   	leave
80102c9a:	c3                   	ret
80102c9b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi

80102ca0 <initlog>:
{
80102ca0:	55                   	push   %ebp
80102ca1:	89 e5                	mov    %esp,%ebp
80102ca3:	53                   	push   %ebx
80102ca4:	83 ec 2c             	sub    $0x2c,%esp
80102ca7:	8b 5d 08             	mov    0x8(%ebp),%ebx
  initlock(&log.lock, "log");
80102caa:	68 37 73 10 80       	push   $0x80107337
80102caf:	68 a0 16 11 80       	push   $0x801116a0
80102cb4:	e8 f7 16 00 00       	call   801043b0 <initlock>
  readsb(dev, &sb);
80102cb9:	58                   	pop    %eax
80102cba:	8d 45 dc             	lea    -0x24(%ebp),%eax
80102cbd:	5a                   	pop    %edx
80102cbe:	50                   	push   %eax
80102cbf:	53                   	push   %ebx
80102cc0:	e8 7b e8 ff ff       	call   80101540 <readsb>
  log.start = sb.logstart;
80102cc5:	8b 45 ec             	mov    -0x14(%ebp),%eax
  struct buf *buf = bread(log.dev, log.start);
80102cc8:	59                   	pop    %ecx
  log.dev = dev;
80102cc9:	89 1d e4 16 11 80    	mov    %ebx,0x801116e4
  log.size = sb.nlog;
80102ccf:	8b 55 e8             	mov    -0x18(%ebp),%edx
  log.start = sb.logstart;
80102cd2:	a3 d4 16 11 80       	mov    %eax,0x801116d4
  log.size = sb.nlog;
80102cd7:	89 15 d8 16 11 80    	mov    %edx,0x801116d8
  struct buf *buf = bread(log.dev, log.start);
80102cdd:	5a                   	pop    %edx
80102cde:	50                   	push   %eax
80102cdf:	53                   	push   %ebx
80102ce0:	e8 eb d3 ff ff       	call   801000d0 <bread>
  for (i = 0; i < log.lh.n; i++) {
80102ce5:	83 c4 10             	add    $0x10,%esp
  log.lh.n = lh->n;
80102ce8:	8b 58 5c             	mov    0x5c(%eax),%ebx
80102ceb:	89 1d e8 16 11 80    	mov    %ebx,0x801116e8
  for (i = 0; i < log.lh.n; i++) {
80102cf1:	85 db                	test   %ebx,%ebx
80102cf3:	7e 1d                	jle    80102d12 <initlog+0x72>
80102cf5:	31 d2                	xor    %edx,%edx
80102cf7:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80102cfe:	00 
80102cff:	90                   	nop
    log.lh.block[i] = lh->block[i];
80102d00:	8b 4c 90 60          	mov    0x60(%eax,%edx,4),%ecx
80102d04:	89 0c 95 ec 16 11 80 	mov    %ecx,-0x7feee914(,%edx,4)
  for (i = 0; i < log.lh.n; i++) {
80102d0b:	83 c2 01             	add    $0x1,%edx
80102d0e:	39 d3                	cmp    %edx,%ebx
80102d10:	75 ee                	jne    80102d00 <initlog+0x60>
  brelse(buf);
80102d12:	83 ec 0c             	sub    $0xc,%esp
80102d15:	50                   	push   %eax
80102d16:	e8 d5 d4 ff ff       	call   801001f0 <brelse>

static void
recover_from_log(void)
{
  read_head();
  install_trans(); // if committed, copy from log to disk
80102d1b:	e8 80 fe ff ff       	call   80102ba0 <install_trans>
  log.lh.n = 0;
80102d20:	c7 05 e8 16 11 80 00 	movl   $0x0,0x801116e8
80102d27:	00 00 00 
  write_head(); // clear the log
80102d2a:	e8 11 ff ff ff       	call   80102c40 <write_head>
}
80102d2f:	8b 5d fc             	mov    -0x4(%ebp),%ebx
80102d32:	83 c4 10             	add    $0x10,%esp
80102d35:	c9                   	leave
80102d36:	c3                   	ret
80102d37:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80102d3e:	00 
80102d3f:	90                   	nop

80102d40 <begin_op>:
}

// called at the start of each FS system call.
void
begin_op(void)
{
80102d40:	55                   	push   %ebp
80102d41:	89 e5                	mov    %esp,%ebp
80102d43:	83 ec 14             	sub    $0x14,%esp
  acquire(&log.lock);
80102d46:	68 a0 16 11 80       	push   $0x801116a0
80102d4b:	e8 50 18 00 00       	call   801045a0 <acquire>
80102d50:	83 c4 10             	add    $0x10,%esp
80102d53:	eb 18                	jmp    80102d6d <begin_op+0x2d>
80102d55:	8d 76 00             	lea    0x0(%esi),%esi
  while(1){
    if(log.committing){
      sleep(&log, &log.lock);
80102d58:	83 ec 08             	sub    $0x8,%esp
80102d5b:	68 a0 16 11 80       	push   $0x801116a0
80102d60:	68 a0 16 11 80       	push   $0x801116a0
80102d65:	e8 b6 12 00 00       	call   80104020 <sleep>
80102d6a:	83 c4 10             	add    $0x10,%esp
    if(log.committing){
80102d6d:	a1 e0 16 11 80       	mov    0x801116e0,%eax
80102d72:	85 c0                	test   %eax,%eax
80102d74:	75 e2                	jne    80102d58 <begin_op+0x18>
    } else if(log.lh.n + (log.outstanding+1)*MAXOPBLOCKS > LOGSIZE){
80102d76:	a1 dc 16 11 80       	mov    0x801116dc,%eax
80102d7b:	8b 15 e8 16 11 80    	mov    0x801116e8,%edx
80102d81:	83 c0 01             	add    $0x1,%eax
80102d84:	8d 0c 80             	lea    (%eax,%eax,4),%ecx
80102d87:	8d 14 4a             	lea    (%edx,%ecx,2),%edx
80102d8a:	83 fa 1e             	cmp    $0x1e,%edx
80102d8d:	7f c9                	jg     80102d58 <begin_op+0x18>
      // this op might exhaust log space; wait for commit.
      sleep(&log, &log.lock);
    } else {
      log.outstanding += 1;
      release(&log.lock);
80102d8f:	83 ec 0c             	sub    $0xc,%esp
      log.outstanding += 1;
80102d92:	a3 dc 16 11 80       	mov    %eax,0x801116dc
      release(&log.lock);
80102d97:	68 a0 16 11 80       	push   $0x801116a0
80102d9c:	e8 9f 17 00 00       	call   80104540 <release>
      break;
    }
  }
}
80102da1:	83 c4 10             	add    $0x10,%esp
80102da4:	c9                   	leave
80102da5:	c3                   	ret
80102da6:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80102dad:	00 
80102dae:	66 90                	xchg   %ax,%ax

80102db0 <end_op>:

// called at the end of each FS system call.
// commits if this was the last outstanding operation.
void
end_op(void)
{
80102db0:	55                   	push   %ebp
80102db1:	89 e5                	mov    %esp,%ebp
80102db3:	57                   	push   %edi
80102db4:	56                   	push   %esi
80102db5:	53                   	push   %ebx
80102db6:	83 ec 18             	sub    $0x18,%esp
  int do_commit = 0;

  acquire(&log.lock);
80102db9:	68 a0 16 11 80       	push   $0x801116a0
80102dbe:	e8 dd 17 00 00       	call   801045a0 <acquire>
  log.outstanding -= 1;
80102dc3:	a1 dc 16 11 80       	mov    0x801116dc,%eax
  if(log.committing)
80102dc8:	8b 35 e0 16 11 80    	mov    0x801116e0,%esi
80102dce:	83 c4 10             	add    $0x10,%esp
  log.outstanding -= 1;
80102dd1:	8d 58 ff             	lea    -0x1(%eax),%ebx
80102dd4:	89 1d dc 16 11 80    	mov    %ebx,0x801116dc
  if(log.committing)
80102dda:	85 f6                	test   %esi,%esi
80102ddc:	0f 85 22 01 00 00    	jne    80102f04 <end_op+0x154>
    panic("log.committing");
  if(log.outstanding == 0){
80102de2:	85 db                	test   %ebx,%ebx
80102de4:	0f 85 f6 00 00 00    	jne    80102ee0 <end_op+0x130>
    do_commit = 1;
    log.committing = 1;
80102dea:	c7 05 e0 16 11 80 01 	movl   $0x1,0x801116e0
80102df1:	00 00 00 
    // begin_op() may be waiting for log space,
    // and decrementing log.outstanding has decreased
    // the amount of reserved space.
    wakeup(&log);
  }
  release(&log.lock);
80102df4:	83 ec 0c             	sub    $0xc,%esp
80102df7:	68 a0 16 11 80       	push   $0x801116a0
80102dfc:	e8 3f 17 00 00       	call   80104540 <release>
}

static void
commit()
{
  if (log.lh.n > 0) {
80102e01:	8b 0d e8 16 11 80    	mov    0x801116e8,%ecx
80102e07:	83 c4 10             	add    $0x10,%esp
80102e0a:	85 c9                	test   %ecx,%ecx
80102e0c:	7f 42                	jg     80102e50 <end_op+0xa0>
    acquire(&log.lock);
80102e0e:	83 ec 0c             	sub    $0xc,%esp
80102e11:	68 a0 16 11 80       	push   $0x801116a0
80102e16:	e8 85 17 00 00       	call   801045a0 <acquire>
    log.committing = 0;
80102e1b:	c7 05 e0 16 11 80 00 	movl   $0x0,0x801116e0
80102e22:	00 00 00 
    wakeup(&log);
80102e25:	c7 04 24 a0 16 11 80 	movl   $0x801116a0,(%esp)
80102e2c:	e8 af 12 00 00       	call   801040e0 <wakeup>
    release(&log.lock);
80102e31:	c7 04 24 a0 16 11 80 	movl   $0x801116a0,(%esp)
80102e38:	e8 03 17 00 00       	call   80104540 <release>
80102e3d:	83 c4 10             	add    $0x10,%esp
}
80102e40:	8d 65 f4             	lea    -0xc(%ebp),%esp
80102e43:	5b                   	pop    %ebx
80102e44:	5e                   	pop    %esi
80102e45:	5f                   	pop    %edi
80102e46:	5d                   	pop    %ebp
80102e47:	c3                   	ret
80102e48:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80102e4f:	00 
    struct buf *to = bread(log.dev, log.start+tail+1); // log block
80102e50:	a1 d4 16 11 80       	mov    0x801116d4,%eax
80102e55:	83 ec 08             	sub    $0x8,%esp
80102e58:	01 d8                	add    %ebx,%eax
80102e5a:	83 c0 01             	add    $0x1,%eax
80102e5d:	50                   	push   %eax
80102e5e:	ff 35 e4 16 11 80    	push   0x801116e4
80102e64:	e8 67 d2 ff ff       	call   801000d0 <bread>
80102e69:	89 c6                	mov    %eax,%esi
    struct buf *from = bread(log.dev, log.lh.block[tail]); // cache block
80102e6b:	58                   	pop    %eax
80102e6c:	5a                   	pop    %edx
80102e6d:	ff 34 9d ec 16 11 80 	push   -0x7feee914(,%ebx,4)
80102e74:	ff 35 e4 16 11 80    	push   0x801116e4
  for (tail = 0; tail < log.lh.n; tail++) {
80102e7a:	83 c3 01             	add    $0x1,%ebx
    struct buf *from = bread(log.dev, log.lh.block[tail]); // cache block
80102e7d:	e8 4e d2 ff ff       	call   801000d0 <bread>
    memmove(to->data, from->data, BSIZE);
80102e82:	83 c4 0c             	add    $0xc,%esp
    struct buf *from = bread(log.dev, log.lh.block[tail]); // cache block
80102e85:	89 c7                	mov    %eax,%edi
    memmove(to->data, from->data, BSIZE);
80102e87:	8d 40 5c             	lea    0x5c(%eax),%eax
80102e8a:	68 00 02 00 00       	push   $0x200
80102e8f:	50                   	push   %eax
80102e90:	8d 46 5c             	lea    0x5c(%esi),%eax
80102e93:	50                   	push   %eax
80102e94:	e8 97 18 00 00       	call   80104730 <memmove>
    bwrite(to);  // write the log
80102e99:	89 34 24             	mov    %esi,(%esp)
80102e9c:	e8 0f d3 ff ff       	call   801001b0 <bwrite>
    brelse(from);
80102ea1:	89 3c 24             	mov    %edi,(%esp)
80102ea4:	e8 47 d3 ff ff       	call   801001f0 <brelse>
    brelse(to);
80102ea9:	89 34 24             	mov    %esi,(%esp)
80102eac:	e8 3f d3 ff ff       	call   801001f0 <brelse>
  for (tail = 0; tail < log.lh.n; tail++) {
80102eb1:	83 c4 10             	add    $0x10,%esp
80102eb4:	3b 1d e8 16 11 80    	cmp    0x801116e8,%ebx
80102eba:	7c 94                	jl     80102e50 <end_op+0xa0>
    write_log();     // Write modified blocks from cache to log
    write_head();    // Write header to disk -- the real commit
80102ebc:	e8 7f fd ff ff       	call   80102c40 <write_head>
    install_trans(); // Now install writes to home locations
80102ec1:	e8 da fc ff ff       	call   80102ba0 <install_trans>
    log.lh.n = 0;
80102ec6:	c7 05 e8 16 11 80 00 	movl   $0x0,0x801116e8
80102ecd:	00 00 00 
    write_head();    // Erase the transaction from the log
80102ed0:	e8 6b fd ff ff       	call   80102c40 <write_head>
80102ed5:	e9 34 ff ff ff       	jmp    80102e0e <end_op+0x5e>
80102eda:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    wakeup(&log);
80102ee0:	83 ec 0c             	sub    $0xc,%esp
80102ee3:	68 a0 16 11 80       	push   $0x801116a0
80102ee8:	e8 f3 11 00 00       	call   801040e0 <wakeup>
  release(&log.lock);
80102eed:	c7 04 24 a0 16 11 80 	movl   $0x801116a0,(%esp)
80102ef4:	e8 47 16 00 00       	call   80104540 <release>
80102ef9:	83 c4 10             	add    $0x10,%esp
}
80102efc:	8d 65 f4             	lea    -0xc(%ebp),%esp
80102eff:	5b                   	pop    %ebx
80102f00:	5e                   	pop    %esi
80102f01:	5f                   	pop    %edi
80102f02:	5d                   	pop    %ebp
80102f03:	c3                   	ret
    panic("log.committing");
80102f04:	83 ec 0c             	sub    $0xc,%esp
80102f07:	68 3b 73 10 80       	push   $0x8010733b
80102f0c:	e8 6f d4 ff ff       	call   80100380 <panic>
80102f11:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80102f18:	00 
80102f19:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi

80102f20 <log_write>:
//   modify bp->data[]
//   log_write(bp)
//   brelse(bp)
void
log_write(struct buf *b)
{
80102f20:	55                   	push   %ebp
80102f21:	89 e5                	mov    %esp,%ebp
80102f23:	53                   	push   %ebx
80102f24:	83 ec 04             	sub    $0x4,%esp
  int i;

  if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
80102f27:	8b 15 e8 16 11 80    	mov    0x801116e8,%edx
{
80102f2d:	8b 5d 08             	mov    0x8(%ebp),%ebx
  if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
80102f30:	83 fa 1d             	cmp    $0x1d,%edx
80102f33:	7f 7d                	jg     80102fb2 <log_write+0x92>
80102f35:	a1 d8 16 11 80       	mov    0x801116d8,%eax
80102f3a:	83 e8 01             	sub    $0x1,%eax
80102f3d:	39 c2                	cmp    %eax,%edx
80102f3f:	7d 71                	jge    80102fb2 <log_write+0x92>
    panic("too big a transaction");
  if (log.outstanding < 1)
80102f41:	a1 dc 16 11 80       	mov    0x801116dc,%eax
80102f46:	85 c0                	test   %eax,%eax
80102f48:	7e 75                	jle    80102fbf <log_write+0x9f>
    panic("log_write outside of trans");

  acquire(&log.lock);
80102f4a:	83 ec 0c             	sub    $0xc,%esp
80102f4d:	68 a0 16 11 80       	push   $0x801116a0
80102f52:	e8 49 16 00 00       	call   801045a0 <acquire>
  for (i = 0; i < log.lh.n; i++) {
    if (log.lh.block[i] == b->blockno)   // log absorbtion
80102f57:	8b 4b 08             	mov    0x8(%ebx),%ecx
  for (i = 0; i < log.lh.n; i++) {
80102f5a:	83 c4 10             	add    $0x10,%esp
80102f5d:	31 c0                	xor    %eax,%eax
80102f5f:	8b 15 e8 16 11 80    	mov    0x801116e8,%edx
80102f65:	85 d2                	test   %edx,%edx
80102f67:	7f 0e                	jg     80102f77 <log_write+0x57>
80102f69:	eb 15                	jmp    80102f80 <log_write+0x60>
80102f6b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
80102f70:	83 c0 01             	add    $0x1,%eax
80102f73:	39 c2                	cmp    %eax,%edx
80102f75:	74 29                	je     80102fa0 <log_write+0x80>
    if (log.lh.block[i] == b->blockno)   // log absorbtion
80102f77:	39 0c 85 ec 16 11 80 	cmp    %ecx,-0x7feee914(,%eax,4)
80102f7e:	75 f0                	jne    80102f70 <log_write+0x50>
      break;
  }
  log.lh.block[i] = b->blockno;
80102f80:	89 0c 85 ec 16 11 80 	mov    %ecx,-0x7feee914(,%eax,4)
  if (i == log.lh.n)
80102f87:	39 c2                	cmp    %eax,%edx
80102f89:	74 1c                	je     80102fa7 <log_write+0x87>
    log.lh.n++;
  b->flags |= B_DIRTY; // prevent eviction
80102f8b:	83 0b 04             	orl    $0x4,(%ebx)
  release(&log.lock);
}
80102f8e:	8b 5d fc             	mov    -0x4(%ebp),%ebx
  release(&log.lock);
80102f91:	c7 45 08 a0 16 11 80 	movl   $0x801116a0,0x8(%ebp)
}
80102f98:	c9                   	leave
  release(&log.lock);
80102f99:	e9 a2 15 00 00       	jmp    80104540 <release>
80102f9e:	66 90                	xchg   %ax,%ax
  log.lh.block[i] = b->blockno;
80102fa0:	89 0c 95 ec 16 11 80 	mov    %ecx,-0x7feee914(,%edx,4)
    log.lh.n++;
80102fa7:	83 c2 01             	add    $0x1,%edx
80102faa:	89 15 e8 16 11 80    	mov    %edx,0x801116e8
80102fb0:	eb d9                	jmp    80102f8b <log_write+0x6b>
    panic("too big a transaction");
80102fb2:	83 ec 0c             	sub    $0xc,%esp
80102fb5:	68 4a 73 10 80       	push   $0x8010734a
80102fba:	e8 c1 d3 ff ff       	call   80100380 <panic>
    panic("log_write outside of trans");
80102fbf:	83 ec 0c             	sub    $0xc,%esp
80102fc2:	68 60 73 10 80       	push   $0x80107360
80102fc7:	e8 b4 d3 ff ff       	call   80100380 <panic>
80102fcc:	66 90                	xchg   %ax,%ax
80102fce:	66 90                	xchg   %ax,%ax

80102fd0 <mpmain>:
}

// Common CPU setup code.
static void
mpmain(void)
{
80102fd0:	55                   	push   %ebp
80102fd1:	89 e5                	mov    %esp,%ebp
80102fd3:	53                   	push   %ebx
80102fd4:	83 ec 04             	sub    $0x4,%esp
  cprintf("cpu%d: starting %d\n", cpuid(), cpuid());
80102fd7:	e8 64 09 00 00       	call   80103940 <cpuid>
80102fdc:	89 c3                	mov    %eax,%ebx
80102fde:	e8 5d 09 00 00       	call   80103940 <cpuid>
80102fe3:	83 ec 04             	sub    $0x4,%esp
80102fe6:	53                   	push   %ebx
80102fe7:	50                   	push   %eax
80102fe8:	68 7b 73 10 80       	push   $0x8010737b
80102fed:	e8 be d6 ff ff       	call   801006b0 <cprintf>
  idtinit();       // load idt register
80102ff2:	e8 e9 28 00 00       	call   801058e0 <idtinit>
  xchg(&(mycpu()->started), 1); // tell startothers() we're up
80102ff7:	e8 e4 08 00 00       	call   801038e0 <mycpu>
80102ffc:	89 c2                	mov    %eax,%edx
xchg(volatile uint *addr, uint newval)
{
  uint result;

  // The + in "+m" denotes a read-modify-write operand.
  asm volatile("lock; xchgl %0, %1" :
80102ffe:	b8 01 00 00 00       	mov    $0x1,%eax
80103003:	f0 87 82 a0 00 00 00 	lock xchg %eax,0xa0(%edx)
  scheduler();     // start running processes
8010300a:	e8 01 0c 00 00       	call   80103c10 <scheduler>
8010300f:	90                   	nop

80103010 <mpenter>:
{
80103010:	55                   	push   %ebp
80103011:	89 e5                	mov    %esp,%ebp
80103013:	83 ec 08             	sub    $0x8,%esp
  switchkvm();
80103016:	e8 c5 39 00 00       	call   801069e0 <switchkvm>
  seginit();
8010301b:	e8 30 39 00 00       	call   80106950 <seginit>
  lapicinit();
80103020:	e8 bb f7 ff ff       	call   801027e0 <lapicinit>
  mpmain();
80103025:	e8 a6 ff ff ff       	call   80102fd0 <mpmain>
8010302a:	66 90                	xchg   %ax,%ax
8010302c:	66 90                	xchg   %ax,%ax
8010302e:	66 90                	xchg   %ax,%ax

80103030 <main>:
{
80103030:	8d 4c 24 04          	lea    0x4(%esp),%ecx
80103034:	83 e4 f0             	and    $0xfffffff0,%esp
80103037:	ff 71 fc             	push   -0x4(%ecx)
8010303a:	55                   	push   %ebp
8010303b:	89 e5                	mov    %esp,%ebp
8010303d:	53                   	push   %ebx
8010303e:	51                   	push   %ecx
  kinit1(end, P2V(4*1024*1024)); // phys page allocator
8010303f:	83 ec 08             	sub    $0x8,%esp
80103042:	68 00 00 40 80       	push   $0x80400000
80103047:	68 d0 54 11 80       	push   $0x801154d0
8010304c:	e8 9f f5 ff ff       	call   801025f0 <kinit1>
  kvmalloc();      // kernel page table
80103051:	e8 4a 3e 00 00       	call   80106ea0 <kvmalloc>
  mpinit();        // detect other processors
80103056:	e8 85 01 00 00       	call   801031e0 <mpinit>
  lapicinit();     // interrupt controller
8010305b:	e8 80 f7 ff ff       	call   801027e0 <lapicinit>
  seginit();       // segment descriptors
80103060:	e8 eb 38 00 00       	call   80106950 <seginit>
  picinit();       // disable pic
80103065:	e8 86 03 00 00       	call   801033f0 <picinit>
  ioapicinit();    // another interrupt controller
8010306a:	e8 51 f3 ff ff       	call   801023c0 <ioapicinit>
  consoleinit();   // console hardware
8010306f:	e8 ec d9 ff ff       	call   80100a60 <consoleinit>
  uartinit();      // serial port
80103074:	e8 47 2b 00 00       	call   80105bc0 <uartinit>
  pinit();         // process table
80103079:	e8 42 08 00 00       	call   801038c0 <pinit>
  tvinit();        // trap vectors
8010307e:	e8 dd 27 00 00       	call   80105860 <tvinit>
  binit();         // buffer cache
80103083:	e8 b8 cf ff ff       	call   80100040 <binit>
  fileinit();      // file table
80103088:	e8 a3 dd ff ff       	call   80100e30 <fileinit>
  ideinit();       // disk 
8010308d:	e8 0e f1 ff ff       	call   801021a0 <ideinit>

  // Write entry code to unused memory at 0x7000.
  // The linker has placed the image of entryother.S in
  // _binary_entryother_start.
  code = P2V(0x7000);
  memmove(code, _binary_entryother_start, (uint)_binary_entryother_size);
80103092:	83 c4 0c             	add    $0xc,%esp
80103095:	68 8a 00 00 00       	push   $0x8a
8010309a:	68 8c a4 10 80       	push   $0x8010a48c
8010309f:	68 00 70 00 80       	push   $0x80007000
801030a4:	e8 87 16 00 00       	call   80104730 <memmove>

  for(c = cpus; c < cpus+ncpu; c++){
801030a9:	83 c4 10             	add    $0x10,%esp
801030ac:	69 05 84 17 11 80 b0 	imul   $0xb0,0x80111784,%eax
801030b3:	00 00 00 
801030b6:	05 a0 17 11 80       	add    $0x801117a0,%eax
801030bb:	3d a0 17 11 80       	cmp    $0x801117a0,%eax
801030c0:	76 7e                	jbe    80103140 <main+0x110>
801030c2:	bb a0 17 11 80       	mov    $0x801117a0,%ebx
801030c7:	eb 20                	jmp    801030e9 <main+0xb9>
801030c9:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
801030d0:	69 05 84 17 11 80 b0 	imul   $0xb0,0x80111784,%eax
801030d7:	00 00 00 
801030da:	81 c3 b0 00 00 00    	add    $0xb0,%ebx
801030e0:	05 a0 17 11 80       	add    $0x801117a0,%eax
801030e5:	39 c3                	cmp    %eax,%ebx
801030e7:	73 57                	jae    80103140 <main+0x110>
    if(c == mycpu())  // We've started already.
801030e9:	e8 f2 07 00 00       	call   801038e0 <mycpu>
801030ee:	39 c3                	cmp    %eax,%ebx
801030f0:	74 de                	je     801030d0 <main+0xa0>
      continue;

    // Tell entryother.S what stack to use, where to enter, and what
    // pgdir to use. We cannot use kpgdir yet, because the AP processor
    // is running in low  memory, so we use entrypgdir for the APs too.
    stack = kalloc();
801030f2:	e8 69 f5 ff ff       	call   80102660 <kalloc>
    *(void**)(code-4) = stack + KSTACKSIZE;
    *(void(**)(void))(code-8) = mpenter;
    *(int**)(code-12) = (void *) V2P(entrypgdir);

    lapicstartap(c->apicid, V2P(code));
801030f7:	83 ec 08             	sub    $0x8,%esp
    *(void(**)(void))(code-8) = mpenter;
801030fa:	c7 05 f8 6f 00 80 10 	movl   $0x80103010,0x80006ff8
80103101:	30 10 80 
    *(int**)(code-12) = (void *) V2P(entrypgdir);
80103104:	c7 05 f4 6f 00 80 00 	movl   $0x109000,0x80006ff4
8010310b:	90 10 00 
    *(void**)(code-4) = stack + KSTACKSIZE;
8010310e:	05 00 10 00 00       	add    $0x1000,%eax
80103113:	a3 fc 6f 00 80       	mov    %eax,0x80006ffc
    lapicstartap(c->apicid, V2P(code));
80103118:	0f b6 03             	movzbl (%ebx),%eax
8010311b:	68 00 70 00 00       	push   $0x7000
80103120:	50                   	push   %eax
80103121:	e8 fa f7 ff ff       	call   80102920 <lapicstartap>

    // wait for cpu to finish mpmain()
    while(c->started == 0)
80103126:	83 c4 10             	add    $0x10,%esp
80103129:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
80103130:	8b 83 a0 00 00 00    	mov    0xa0(%ebx),%eax
80103136:	85 c0                	test   %eax,%eax
80103138:	74 f6                	je     80103130 <main+0x100>
8010313a:	eb 94                	jmp    801030d0 <main+0xa0>
8010313c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
  kinit2(P2V(4*1024*1024), P2V(PHYSTOP)); // must come after startothers()
80103140:	83 ec 08             	sub    $0x8,%esp
80103143:	68 00 00 00 8e       	push   $0x8e000000
80103148:	68 00 00 40 80       	push   $0x80400000
8010314d:	e8 3e f4 ff ff       	call   80102590 <kinit2>
  userinit();      // first user process
80103152:	e8 39 08 00 00       	call   80103990 <userinit>
  mpmain();        // finish this processor's setup
80103157:	e8 74 fe ff ff       	call   80102fd0 <mpmain>
8010315c:	66 90                	xchg   %ax,%ax
8010315e:	66 90                	xchg   %ax,%ax

80103160 <mpsearch1>:
}

// Look for an MP structure in the len bytes at addr.
static struct mp*
mpsearch1(uint a, int len)
{
80103160:	55                   	push   %ebp
80103161:	89 e5                	mov    %esp,%ebp
80103163:	57                   	push   %edi
80103164:	56                   	push   %esi
  uchar *e, *p, *addr;

  addr = P2V(a);
80103165:	8d b0 00 00 00 80    	lea    -0x80000000(%eax),%esi
{
8010316b:	53                   	push   %ebx
  e = addr+len;
8010316c:	8d 1c 16             	lea    (%esi,%edx,1),%ebx
{
8010316f:	83 ec 0c             	sub    $0xc,%esp
  for(p = addr; p < e; p += sizeof(struct mp))
80103172:	39 de                	cmp    %ebx,%esi
80103174:	72 10                	jb     80103186 <mpsearch1+0x26>
80103176:	eb 50                	jmp    801031c8 <mpsearch1+0x68>
80103178:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010317f:	00 
80103180:	89 fe                	mov    %edi,%esi
80103182:	39 df                	cmp    %ebx,%edi
80103184:	73 42                	jae    801031c8 <mpsearch1+0x68>
    if(memcmp(p, "_MP_", 4) == 0 && sum(p, sizeof(struct mp)) == 0)
80103186:	83 ec 04             	sub    $0x4,%esp
80103189:	8d 7e 10             	lea    0x10(%esi),%edi
8010318c:	6a 04                	push   $0x4
8010318e:	68 8f 73 10 80       	push   $0x8010738f
80103193:	56                   	push   %esi
80103194:	e8 47 15 00 00       	call   801046e0 <memcmp>
80103199:	83 c4 10             	add    $0x10,%esp
8010319c:	85 c0                	test   %eax,%eax
8010319e:	75 e0                	jne    80103180 <mpsearch1+0x20>
801031a0:	89 f2                	mov    %esi,%edx
801031a2:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    sum += addr[i];
801031a8:	0f b6 0a             	movzbl (%edx),%ecx
  for(i=0; i<len; i++)
801031ab:	83 c2 01             	add    $0x1,%edx
    sum += addr[i];
801031ae:	01 c8                	add    %ecx,%eax
  for(i=0; i<len; i++)
801031b0:	39 fa                	cmp    %edi,%edx
801031b2:	75 f4                	jne    801031a8 <mpsearch1+0x48>
    if(memcmp(p, "_MP_", 4) == 0 && sum(p, sizeof(struct mp)) == 0)
801031b4:	84 c0                	test   %al,%al
801031b6:	75 c8                	jne    80103180 <mpsearch1+0x20>
      return (struct mp*)p;
  return 0;
}
801031b8:	8d 65 f4             	lea    -0xc(%ebp),%esp
801031bb:	89 f0                	mov    %esi,%eax
801031bd:	5b                   	pop    %ebx
801031be:	5e                   	pop    %esi
801031bf:	5f                   	pop    %edi
801031c0:	5d                   	pop    %ebp
801031c1:	c3                   	ret
801031c2:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
801031c8:	8d 65 f4             	lea    -0xc(%ebp),%esp
  return 0;
801031cb:	31 f6                	xor    %esi,%esi
}
801031cd:	5b                   	pop    %ebx
801031ce:	89 f0                	mov    %esi,%eax
801031d0:	5e                   	pop    %esi
801031d1:	5f                   	pop    %edi
801031d2:	5d                   	pop    %ebp
801031d3:	c3                   	ret
801031d4:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801031db:	00 
801031dc:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi

801031e0 <mpinit>:
  return conf;
}

void
mpinit(void)
{
801031e0:	55                   	push   %ebp
801031e1:	89 e5                	mov    %esp,%ebp
801031e3:	57                   	push   %edi
801031e4:	56                   	push   %esi
801031e5:	53                   	push   %ebx
801031e6:	83 ec 1c             	sub    $0x1c,%esp
  if((p = ((bda[0x0F]<<8)| bda[0x0E]) << 4)){
801031e9:	0f b6 05 0f 04 00 80 	movzbl 0x8000040f,%eax
801031f0:	0f b6 15 0e 04 00 80 	movzbl 0x8000040e,%edx
801031f7:	c1 e0 08             	shl    $0x8,%eax
801031fa:	09 d0                	or     %edx,%eax
801031fc:	c1 e0 04             	shl    $0x4,%eax
801031ff:	75 1b                	jne    8010321c <mpinit+0x3c>
    p = ((bda[0x14]<<8)|bda[0x13])*1024;
80103201:	0f b6 05 14 04 00 80 	movzbl 0x80000414,%eax
80103208:	0f b6 15 13 04 00 80 	movzbl 0x80000413,%edx
8010320f:	c1 e0 08             	shl    $0x8,%eax
80103212:	09 d0                	or     %edx,%eax
80103214:	c1 e0 0a             	shl    $0xa,%eax
    if((mp = mpsearch1(p-1024, 1024)))
80103217:	2d 00 04 00 00       	sub    $0x400,%eax
    if((mp = mpsearch1(p, 1024)))
8010321c:	ba 00 04 00 00       	mov    $0x400,%edx
80103221:	e8 3a ff ff ff       	call   80103160 <mpsearch1>
80103226:	89 c3                	mov    %eax,%ebx
80103228:	85 c0                	test   %eax,%eax
8010322a:	0f 84 58 01 00 00    	je     80103388 <mpinit+0x1a8>
  if((mp = mpsearch()) == 0 || mp->physaddr == 0)
80103230:	8b 73 04             	mov    0x4(%ebx),%esi
80103233:	85 f6                	test   %esi,%esi
80103235:	0f 84 3d 01 00 00    	je     80103378 <mpinit+0x198>
  if(memcmp(conf, "PCMP", 4) != 0)
8010323b:	83 ec 04             	sub    $0x4,%esp
  conf = (struct mpconf*) P2V((uint) mp->physaddr);
8010323e:	8d 86 00 00 00 80    	lea    -0x80000000(%esi),%eax
80103244:	89 45 e4             	mov    %eax,-0x1c(%ebp)
  if(memcmp(conf, "PCMP", 4) != 0)
80103247:	6a 04                	push   $0x4
80103249:	68 94 73 10 80       	push   $0x80107394
8010324e:	50                   	push   %eax
8010324f:	e8 8c 14 00 00       	call   801046e0 <memcmp>
80103254:	83 c4 10             	add    $0x10,%esp
80103257:	85 c0                	test   %eax,%eax
80103259:	0f 85 19 01 00 00    	jne    80103378 <mpinit+0x198>
  if(conf->version != 1 && conf->version != 4)
8010325f:	0f b6 86 06 00 00 80 	movzbl -0x7ffffffa(%esi),%eax
80103266:	3c 01                	cmp    $0x1,%al
80103268:	74 08                	je     80103272 <mpinit+0x92>
8010326a:	3c 04                	cmp    $0x4,%al
8010326c:	0f 85 06 01 00 00    	jne    80103378 <mpinit+0x198>
  if(sum((uchar*)conf, conf->length) != 0)
80103272:	0f b7 96 04 00 00 80 	movzwl -0x7ffffffc(%esi),%edx
  for(i=0; i<len; i++)
80103279:	66 85 d2             	test   %dx,%dx
8010327c:	74 22                	je     801032a0 <mpinit+0xc0>
8010327e:	8d 3c 32             	lea    (%edx,%esi,1),%edi
80103281:	89 f0                	mov    %esi,%eax
  sum = 0;
80103283:	31 d2                	xor    %edx,%edx
80103285:	8d 76 00             	lea    0x0(%esi),%esi
    sum += addr[i];
80103288:	0f b6 88 00 00 00 80 	movzbl -0x80000000(%eax),%ecx
  for(i=0; i<len; i++)
8010328f:	83 c0 01             	add    $0x1,%eax
    sum += addr[i];
80103292:	01 ca                	add    %ecx,%edx
  for(i=0; i<len; i++)
80103294:	39 f8                	cmp    %edi,%eax
80103296:	75 f0                	jne    80103288 <mpinit+0xa8>
  if(sum((uchar*)conf, conf->length) != 0)
80103298:	84 d2                	test   %dl,%dl
8010329a:	0f 85 d8 00 00 00    	jne    80103378 <mpinit+0x198>
  struct mpioapic *ioapic;

  if((conf = mpconfig(&mp)) == 0)
    panic("Expect to run on an SMP");
  ismp = 1;
  lapic = (uint*)conf->lapicaddr;
801032a0:	8b 86 24 00 00 80    	mov    -0x7fffffdc(%esi),%eax
  for(p=(uchar*)(conf+1), e=(uchar*)conf+conf->length; p<e; ){
801032a6:	8b 7d e4             	mov    -0x1c(%ebp),%edi
801032a9:	89 5d e4             	mov    %ebx,-0x1c(%ebp)
  lapic = (uint*)conf->lapicaddr;
801032ac:	a3 80 16 11 80       	mov    %eax,0x80111680
  for(p=(uchar*)(conf+1), e=(uchar*)conf+conf->length; p<e; ){
801032b1:	0f b7 96 04 00 00 80 	movzwl -0x7ffffffc(%esi),%edx
801032b8:	8d 86 2c 00 00 80    	lea    -0x7fffffd4(%esi),%eax
801032be:	01 d7                	add    %edx,%edi
801032c0:	89 fa                	mov    %edi,%edx
  ismp = 1;
801032c2:	bf 01 00 00 00       	mov    $0x1,%edi
801032c7:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801032ce:	00 
801032cf:	90                   	nop
  for(p=(uchar*)(conf+1), e=(uchar*)conf+conf->length; p<e; ){
801032d0:	39 d0                	cmp    %edx,%eax
801032d2:	73 19                	jae    801032ed <mpinit+0x10d>
    switch(*p){
801032d4:	0f b6 08             	movzbl (%eax),%ecx
801032d7:	80 f9 02             	cmp    $0x2,%cl
801032da:	0f 84 80 00 00 00    	je     80103360 <mpinit+0x180>
801032e0:	77 6e                	ja     80103350 <mpinit+0x170>
801032e2:	84 c9                	test   %cl,%cl
801032e4:	74 3a                	je     80103320 <mpinit+0x140>
      p += sizeof(struct mpioapic);
      continue;
    case MPBUS:
    case MPIOINTR:
    case MPLINTR:
      p += 8;
801032e6:	83 c0 08             	add    $0x8,%eax
  for(p=(uchar*)(conf+1), e=(uchar*)conf+conf->length; p<e; ){
801032e9:	39 d0                	cmp    %edx,%eax
801032eb:	72 e7                	jb     801032d4 <mpinit+0xf4>
    default:
      ismp = 0;
      break;
    }
  }
  if(!ismp)
801032ed:	8b 5d e4             	mov    -0x1c(%ebp),%ebx
801032f0:	85 ff                	test   %edi,%edi
801032f2:	0f 84 dd 00 00 00    	je     801033d5 <mpinit+0x1f5>
    panic("Didn't find a suitable machine");

  if(mp->imcrp){
801032f8:	80 7b 0c 00          	cmpb   $0x0,0xc(%ebx)
801032fc:	74 15                	je     80103313 <mpinit+0x133>
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
801032fe:	b8 70 00 00 00       	mov    $0x70,%eax
80103303:	ba 22 00 00 00       	mov    $0x22,%edx
80103308:	ee                   	out    %al,(%dx)
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
80103309:	ba 23 00 00 00       	mov    $0x23,%edx
8010330e:	ec                   	in     (%dx),%al
    // Bochs doesn't support IMCR, so this doesn't run on Bochs.
    // But it would on real hardware.
    outb(0x22, 0x70);   // Select IMCR
    outb(0x23, inb(0x23) | 1);  // Mask external interrupts.
8010330f:	83 c8 01             	or     $0x1,%eax
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
80103312:	ee                   	out    %al,(%dx)
  }
}
80103313:	8d 65 f4             	lea    -0xc(%ebp),%esp
80103316:	5b                   	pop    %ebx
80103317:	5e                   	pop    %esi
80103318:	5f                   	pop    %edi
80103319:	5d                   	pop    %ebp
8010331a:	c3                   	ret
8010331b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
      if(ncpu < NCPU) {
80103320:	8b 0d 84 17 11 80    	mov    0x80111784,%ecx
80103326:	83 f9 07             	cmp    $0x7,%ecx
80103329:	7f 19                	jg     80103344 <mpinit+0x164>
        cpus[ncpu].apicid = proc->apicid;  // apicid may differ from ncpu
8010332b:	69 f1 b0 00 00 00    	imul   $0xb0,%ecx,%esi
80103331:	0f b6 58 01          	movzbl 0x1(%eax),%ebx
        ncpu++;
80103335:	83 c1 01             	add    $0x1,%ecx
80103338:	89 0d 84 17 11 80    	mov    %ecx,0x80111784
        cpus[ncpu].apicid = proc->apicid;  // apicid may differ from ncpu
8010333e:	88 9e a0 17 11 80    	mov    %bl,-0x7feee860(%esi)
      p += sizeof(struct mpproc);
80103344:	83 c0 14             	add    $0x14,%eax
      continue;
80103347:	eb 87                	jmp    801032d0 <mpinit+0xf0>
80103349:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    switch(*p){
80103350:	83 e9 03             	sub    $0x3,%ecx
80103353:	80 f9 01             	cmp    $0x1,%cl
80103356:	76 8e                	jbe    801032e6 <mpinit+0x106>
80103358:	31 ff                	xor    %edi,%edi
8010335a:	e9 71 ff ff ff       	jmp    801032d0 <mpinit+0xf0>
8010335f:	90                   	nop
      ioapicid = ioapic->apicno;
80103360:	0f b6 48 01          	movzbl 0x1(%eax),%ecx
      p += sizeof(struct mpioapic);
80103364:	83 c0 08             	add    $0x8,%eax
      ioapicid = ioapic->apicno;
80103367:	88 0d 80 17 11 80    	mov    %cl,0x80111780
      continue;
8010336d:	e9 5e ff ff ff       	jmp    801032d0 <mpinit+0xf0>
80103372:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    panic("Expect to run on an SMP");
80103378:	83 ec 0c             	sub    $0xc,%esp
8010337b:	68 99 73 10 80       	push   $0x80107399
80103380:	e8 fb cf ff ff       	call   80100380 <panic>
80103385:	8d 76 00             	lea    0x0(%esi),%esi
{
80103388:	bb 00 00 0f 80       	mov    $0x800f0000,%ebx
8010338d:	eb 0b                	jmp    8010339a <mpinit+0x1ba>
8010338f:	90                   	nop
  for(p = addr; p < e; p += sizeof(struct mp))
80103390:	89 f3                	mov    %esi,%ebx
80103392:	81 fe 00 00 10 80    	cmp    $0x80100000,%esi
80103398:	74 de                	je     80103378 <mpinit+0x198>
    if(memcmp(p, "_MP_", 4) == 0 && sum(p, sizeof(struct mp)) == 0)
8010339a:	83 ec 04             	sub    $0x4,%esp
8010339d:	8d 73 10             	lea    0x10(%ebx),%esi
801033a0:	6a 04                	push   $0x4
801033a2:	68 8f 73 10 80       	push   $0x8010738f
801033a7:	53                   	push   %ebx
801033a8:	e8 33 13 00 00       	call   801046e0 <memcmp>
801033ad:	83 c4 10             	add    $0x10,%esp
801033b0:	85 c0                	test   %eax,%eax
801033b2:	75 dc                	jne    80103390 <mpinit+0x1b0>
801033b4:	89 da                	mov    %ebx,%edx
801033b6:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801033bd:	00 
801033be:	66 90                	xchg   %ax,%ax
    sum += addr[i];
801033c0:	0f b6 0a             	movzbl (%edx),%ecx
  for(i=0; i<len; i++)
801033c3:	83 c2 01             	add    $0x1,%edx
    sum += addr[i];
801033c6:	01 c8                	add    %ecx,%eax
  for(i=0; i<len; i++)
801033c8:	39 d6                	cmp    %edx,%esi
801033ca:	75 f4                	jne    801033c0 <mpinit+0x1e0>
    if(memcmp(p, "_MP_", 4) == 0 && sum(p, sizeof(struct mp)) == 0)
801033cc:	84 c0                	test   %al,%al
801033ce:	75 c0                	jne    80103390 <mpinit+0x1b0>
801033d0:	e9 5b fe ff ff       	jmp    80103230 <mpinit+0x50>
    panic("Didn't find a suitable machine");
801033d5:	83 ec 0c             	sub    $0xc,%esp
801033d8:	68 fc 76 10 80       	push   $0x801076fc
801033dd:	e8 9e cf ff ff       	call   80100380 <panic>
801033e2:	66 90                	xchg   %ax,%ax
801033e4:	66 90                	xchg   %ax,%ax
801033e6:	66 90                	xchg   %ax,%ax
801033e8:	66 90                	xchg   %ax,%ax
801033ea:	66 90                	xchg   %ax,%ax
801033ec:	66 90                	xchg   %ax,%ax
801033ee:	66 90                	xchg   %ax,%ax

801033f0 <picinit>:
801033f0:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
801033f5:	ba 21 00 00 00       	mov    $0x21,%edx
801033fa:	ee                   	out    %al,(%dx)
801033fb:	ba a1 00 00 00       	mov    $0xa1,%edx
80103400:	ee                   	out    %al,(%dx)
picinit(void)
{
  // mask all interrupts
  outb(IO_PIC1+1, 0xFF);
  outb(IO_PIC2+1, 0xFF);
}
80103401:	c3                   	ret
80103402:	66 90                	xchg   %ax,%ax
80103404:	66 90                	xchg   %ax,%ax
80103406:	66 90                	xchg   %ax,%ax
80103408:	66 90                	xchg   %ax,%ax
8010340a:	66 90                	xchg   %ax,%ax
8010340c:	66 90                	xchg   %ax,%ax
8010340e:	66 90                	xchg   %ax,%ax

80103410 <pipealloc>:
  int writeopen;  // write fd is still open
};

int
pipealloc(struct file **f0, struct file **f1)
{
80103410:	55                   	push   %ebp
80103411:	89 e5                	mov    %esp,%ebp
80103413:	57                   	push   %edi
80103414:	56                   	push   %esi
80103415:	53                   	push   %ebx
80103416:	83 ec 0c             	sub    $0xc,%esp
80103419:	8b 75 08             	mov    0x8(%ebp),%esi
8010341c:	8b 7d 0c             	mov    0xc(%ebp),%edi
  struct pipe *p;

  p = 0;
  *f0 = *f1 = 0;
8010341f:	c7 07 00 00 00 00    	movl   $0x0,(%edi)
80103425:	c7 06 00 00 00 00    	movl   $0x0,(%esi)
  if((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
8010342b:	e8 20 da ff ff       	call   80100e50 <filealloc>
80103430:	89 06                	mov    %eax,(%esi)
80103432:	85 c0                	test   %eax,%eax
80103434:	0f 84 a5 00 00 00    	je     801034df <pipealloc+0xcf>
8010343a:	e8 11 da ff ff       	call   80100e50 <filealloc>
8010343f:	89 07                	mov    %eax,(%edi)
80103441:	85 c0                	test   %eax,%eax
80103443:	0f 84 84 00 00 00    	je     801034cd <pipealloc+0xbd>
    goto bad;
  if((p = (struct pipe*)kalloc()) == 0)
80103449:	e8 12 f2 ff ff       	call   80102660 <kalloc>
8010344e:	89 c3                	mov    %eax,%ebx
80103450:	85 c0                	test   %eax,%eax
80103452:	0f 84 a0 00 00 00    	je     801034f8 <pipealloc+0xe8>
    goto bad;
  p->readopen = 1;
80103458:	c7 80 3c 02 00 00 01 	movl   $0x1,0x23c(%eax)
8010345f:	00 00 00 
  p->writeopen = 1;
  p->nwrite = 0;
  p->nread = 0;
  initlock(&p->lock, "pipe");
80103462:	83 ec 08             	sub    $0x8,%esp
  p->writeopen = 1;
80103465:	c7 80 40 02 00 00 01 	movl   $0x1,0x240(%eax)
8010346c:	00 00 00 
  p->nwrite = 0;
8010346f:	c7 80 38 02 00 00 00 	movl   $0x0,0x238(%eax)
80103476:	00 00 00 
  p->nread = 0;
80103479:	c7 80 34 02 00 00 00 	movl   $0x0,0x234(%eax)
80103480:	00 00 00 
  initlock(&p->lock, "pipe");
80103483:	68 b1 73 10 80       	push   $0x801073b1
80103488:	50                   	push   %eax
80103489:	e8 22 0f 00 00       	call   801043b0 <initlock>
  (*f0)->type = FD_PIPE;
8010348e:	8b 06                	mov    (%esi),%eax
  (*f0)->pipe = p;
  (*f1)->type = FD_PIPE;
  (*f1)->readable = 0;
  (*f1)->writable = 1;
  (*f1)->pipe = p;
  return 0;
80103490:	83 c4 10             	add    $0x10,%esp
  (*f0)->type = FD_PIPE;
80103493:	c7 00 01 00 00 00    	movl   $0x1,(%eax)
  (*f0)->readable = 1;
80103499:	8b 06                	mov    (%esi),%eax
8010349b:	c6 40 08 01          	movb   $0x1,0x8(%eax)
  (*f0)->writable = 0;
8010349f:	8b 06                	mov    (%esi),%eax
801034a1:	c6 40 09 00          	movb   $0x0,0x9(%eax)
  (*f0)->pipe = p;
801034a5:	8b 06                	mov    (%esi),%eax
801034a7:	89 58 0c             	mov    %ebx,0xc(%eax)
  (*f1)->type = FD_PIPE;
801034aa:	8b 07                	mov    (%edi),%eax
801034ac:	c7 00 01 00 00 00    	movl   $0x1,(%eax)
  (*f1)->readable = 0;
801034b2:	8b 07                	mov    (%edi),%eax
801034b4:	c6 40 08 00          	movb   $0x0,0x8(%eax)
  (*f1)->writable = 1;
801034b8:	8b 07                	mov    (%edi),%eax
801034ba:	c6 40 09 01          	movb   $0x1,0x9(%eax)
  (*f1)->pipe = p;
801034be:	8b 07                	mov    (%edi),%eax
801034c0:	89 58 0c             	mov    %ebx,0xc(%eax)
  return 0;
801034c3:	31 c0                	xor    %eax,%eax
  if(*f0)
    fileclose(*f0);
  if(*f1)
    fileclose(*f1);
  return -1;
}
801034c5:	8d 65 f4             	lea    -0xc(%ebp),%esp
801034c8:	5b                   	pop    %ebx
801034c9:	5e                   	pop    %esi
801034ca:	5f                   	pop    %edi
801034cb:	5d                   	pop    %ebp
801034cc:	c3                   	ret
  if(*f0)
801034cd:	8b 06                	mov    (%esi),%eax
801034cf:	85 c0                	test   %eax,%eax
801034d1:	74 1e                	je     801034f1 <pipealloc+0xe1>
    fileclose(*f0);
801034d3:	83 ec 0c             	sub    $0xc,%esp
801034d6:	50                   	push   %eax
801034d7:	e8 34 da ff ff       	call   80100f10 <fileclose>
801034dc:	83 c4 10             	add    $0x10,%esp
  if(*f1)
801034df:	8b 07                	mov    (%edi),%eax
801034e1:	85 c0                	test   %eax,%eax
801034e3:	74 0c                	je     801034f1 <pipealloc+0xe1>
    fileclose(*f1);
801034e5:	83 ec 0c             	sub    $0xc,%esp
801034e8:	50                   	push   %eax
801034e9:	e8 22 da ff ff       	call   80100f10 <fileclose>
801034ee:	83 c4 10             	add    $0x10,%esp
  return -1;
801034f1:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
801034f6:	eb cd                	jmp    801034c5 <pipealloc+0xb5>
  if(*f0)
801034f8:	8b 06                	mov    (%esi),%eax
801034fa:	85 c0                	test   %eax,%eax
801034fc:	75 d5                	jne    801034d3 <pipealloc+0xc3>
801034fe:	eb df                	jmp    801034df <pipealloc+0xcf>

80103500 <pipeclose>:

void
pipeclose(struct pipe *p, int writable)
{
80103500:	55                   	push   %ebp
80103501:	89 e5                	mov    %esp,%ebp
80103503:	56                   	push   %esi
80103504:	53                   	push   %ebx
80103505:	8b 5d 08             	mov    0x8(%ebp),%ebx
80103508:	8b 75 0c             	mov    0xc(%ebp),%esi
  acquire(&p->lock);
8010350b:	83 ec 0c             	sub    $0xc,%esp
8010350e:	53                   	push   %ebx
8010350f:	e8 8c 10 00 00       	call   801045a0 <acquire>
  if(writable){
80103514:	83 c4 10             	add    $0x10,%esp
80103517:	85 f6                	test   %esi,%esi
80103519:	74 65                	je     80103580 <pipeclose+0x80>
    p->writeopen = 0;
    wakeup(&p->nread);
8010351b:	83 ec 0c             	sub    $0xc,%esp
8010351e:	8d 83 34 02 00 00    	lea    0x234(%ebx),%eax
    p->writeopen = 0;
80103524:	c7 83 40 02 00 00 00 	movl   $0x0,0x240(%ebx)
8010352b:	00 00 00 
    wakeup(&p->nread);
8010352e:	50                   	push   %eax
8010352f:	e8 ac 0b 00 00       	call   801040e0 <wakeup>
80103534:	83 c4 10             	add    $0x10,%esp
  } else {
    p->readopen = 0;
    wakeup(&p->nwrite);
  }
  if(p->readopen == 0 && p->writeopen == 0){
80103537:	8b 93 3c 02 00 00    	mov    0x23c(%ebx),%edx
8010353d:	85 d2                	test   %edx,%edx
8010353f:	75 0a                	jne    8010354b <pipeclose+0x4b>
80103541:	8b 83 40 02 00 00    	mov    0x240(%ebx),%eax
80103547:	85 c0                	test   %eax,%eax
80103549:	74 15                	je     80103560 <pipeclose+0x60>
    release(&p->lock);
    kfree((char*)p);
  } else
    release(&p->lock);
8010354b:	89 5d 08             	mov    %ebx,0x8(%ebp)
}
8010354e:	8d 65 f8             	lea    -0x8(%ebp),%esp
80103551:	5b                   	pop    %ebx
80103552:	5e                   	pop    %esi
80103553:	5d                   	pop    %ebp
    release(&p->lock);
80103554:	e9 e7 0f 00 00       	jmp    80104540 <release>
80103559:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    release(&p->lock);
80103560:	83 ec 0c             	sub    $0xc,%esp
80103563:	53                   	push   %ebx
80103564:	e8 d7 0f 00 00       	call   80104540 <release>
    kfree((char*)p);
80103569:	83 c4 10             	add    $0x10,%esp
8010356c:	89 5d 08             	mov    %ebx,0x8(%ebp)
}
8010356f:	8d 65 f8             	lea    -0x8(%ebp),%esp
80103572:	5b                   	pop    %ebx
80103573:	5e                   	pop    %esi
80103574:	5d                   	pop    %ebp
    kfree((char*)p);
80103575:	e9 26 ef ff ff       	jmp    801024a0 <kfree>
8010357a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    wakeup(&p->nwrite);
80103580:	83 ec 0c             	sub    $0xc,%esp
80103583:	8d 83 38 02 00 00    	lea    0x238(%ebx),%eax
    p->readopen = 0;
80103589:	c7 83 3c 02 00 00 00 	movl   $0x0,0x23c(%ebx)
80103590:	00 00 00 
    wakeup(&p->nwrite);
80103593:	50                   	push   %eax
80103594:	e8 47 0b 00 00       	call   801040e0 <wakeup>
80103599:	83 c4 10             	add    $0x10,%esp
8010359c:	eb 99                	jmp    80103537 <pipeclose+0x37>
8010359e:	66 90                	xchg   %ax,%ax

801035a0 <pipewrite>:

//PAGEBREAK: 40
int
pipewrite(struct pipe *p, char *addr, int n)
{
801035a0:	55                   	push   %ebp
801035a1:	89 e5                	mov    %esp,%ebp
801035a3:	57                   	push   %edi
801035a4:	56                   	push   %esi
801035a5:	53                   	push   %ebx
801035a6:	83 ec 28             	sub    $0x28,%esp
801035a9:	8b 5d 08             	mov    0x8(%ebp),%ebx
801035ac:	8b 7d 10             	mov    0x10(%ebp),%edi
  int i;

  acquire(&p->lock);
801035af:	53                   	push   %ebx
801035b0:	e8 eb 0f 00 00       	call   801045a0 <acquire>
  for(i = 0; i < n; i++){
801035b5:	83 c4 10             	add    $0x10,%esp
801035b8:	85 ff                	test   %edi,%edi
801035ba:	0f 8e ce 00 00 00    	jle    8010368e <pipewrite+0xee>
    while(p->nwrite == p->nread + PIPESIZE){  //DOC: pipewrite-full
801035c0:	8b 83 38 02 00 00    	mov    0x238(%ebx),%eax
801035c6:	8b 4d 0c             	mov    0xc(%ebp),%ecx
801035c9:	89 7d 10             	mov    %edi,0x10(%ebp)
801035cc:	89 45 e4             	mov    %eax,-0x1c(%ebp)
801035cf:	8d 34 39             	lea    (%ecx,%edi,1),%esi
801035d2:	89 75 e0             	mov    %esi,-0x20(%ebp)
      if(p->readopen == 0 || myproc()->killed){
        release(&p->lock);
        return -1;
      }
      wakeup(&p->nread);
801035d5:	8d b3 34 02 00 00    	lea    0x234(%ebx),%esi
    while(p->nwrite == p->nread + PIPESIZE){  //DOC: pipewrite-full
801035db:	8b 83 34 02 00 00    	mov    0x234(%ebx),%eax
      sleep(&p->nwrite, &p->lock);  //DOC: pipewrite-sleep
801035e1:	8d bb 38 02 00 00    	lea    0x238(%ebx),%edi
    while(p->nwrite == p->nread + PIPESIZE){  //DOC: pipewrite-full
801035e7:	8d 90 00 02 00 00    	lea    0x200(%eax),%edx
801035ed:	39 55 e4             	cmp    %edx,-0x1c(%ebp)
801035f0:	0f 85 b6 00 00 00    	jne    801036ac <pipewrite+0x10c>
801035f6:	89 4d e4             	mov    %ecx,-0x1c(%ebp)
801035f9:	eb 3b                	jmp    80103636 <pipewrite+0x96>
801035fb:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
      if(p->readopen == 0 || myproc()->killed){
80103600:	e8 5b 03 00 00       	call   80103960 <myproc>
80103605:	8b 48 24             	mov    0x24(%eax),%ecx
80103608:	85 c9                	test   %ecx,%ecx
8010360a:	75 34                	jne    80103640 <pipewrite+0xa0>
      wakeup(&p->nread);
8010360c:	83 ec 0c             	sub    $0xc,%esp
8010360f:	56                   	push   %esi
80103610:	e8 cb 0a 00 00       	call   801040e0 <wakeup>
      sleep(&p->nwrite, &p->lock);  //DOC: pipewrite-sleep
80103615:	58                   	pop    %eax
80103616:	5a                   	pop    %edx
80103617:	53                   	push   %ebx
80103618:	57                   	push   %edi
80103619:	e8 02 0a 00 00       	call   80104020 <sleep>
    while(p->nwrite == p->nread + PIPESIZE){  //DOC: pipewrite-full
8010361e:	8b 83 34 02 00 00    	mov    0x234(%ebx),%eax
80103624:	8b 93 38 02 00 00    	mov    0x238(%ebx),%edx
8010362a:	83 c4 10             	add    $0x10,%esp
8010362d:	05 00 02 00 00       	add    $0x200,%eax
80103632:	39 c2                	cmp    %eax,%edx
80103634:	75 2a                	jne    80103660 <pipewrite+0xc0>
      if(p->readopen == 0 || myproc()->killed){
80103636:	8b 83 3c 02 00 00    	mov    0x23c(%ebx),%eax
8010363c:	85 c0                	test   %eax,%eax
8010363e:	75 c0                	jne    80103600 <pipewrite+0x60>
        release(&p->lock);
80103640:	83 ec 0c             	sub    $0xc,%esp
80103643:	53                   	push   %ebx
80103644:	e8 f7 0e 00 00       	call   80104540 <release>
        return -1;
80103649:	83 c4 10             	add    $0x10,%esp
8010364c:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
    p->data[p->nwrite++ % PIPESIZE] = addr[i];
  }
  wakeup(&p->nread);  //DOC: pipewrite-wakeup1
  release(&p->lock);
  return n;
}
80103651:	8d 65 f4             	lea    -0xc(%ebp),%esp
80103654:	5b                   	pop    %ebx
80103655:	5e                   	pop    %esi
80103656:	5f                   	pop    %edi
80103657:	5d                   	pop    %ebp
80103658:	c3                   	ret
80103659:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
80103660:	8b 4d e4             	mov    -0x1c(%ebp),%ecx
    p->data[p->nwrite++ % PIPESIZE] = addr[i];
80103663:	8d 42 01             	lea    0x1(%edx),%eax
80103666:	81 e2 ff 01 00 00    	and    $0x1ff,%edx
  for(i = 0; i < n; i++){
8010366c:	83 c1 01             	add    $0x1,%ecx
    p->data[p->nwrite++ % PIPESIZE] = addr[i];
8010366f:	89 83 38 02 00 00    	mov    %eax,0x238(%ebx)
80103675:	89 45 e4             	mov    %eax,-0x1c(%ebp)
80103678:	0f b6 41 ff          	movzbl -0x1(%ecx),%eax
8010367c:	88 44 13 34          	mov    %al,0x34(%ebx,%edx,1)
  for(i = 0; i < n; i++){
80103680:	8b 45 e0             	mov    -0x20(%ebp),%eax
80103683:	39 c1                	cmp    %eax,%ecx
80103685:	0f 85 50 ff ff ff    	jne    801035db <pipewrite+0x3b>
8010368b:	8b 7d 10             	mov    0x10(%ebp),%edi
  wakeup(&p->nread);  //DOC: pipewrite-wakeup1
8010368e:	83 ec 0c             	sub    $0xc,%esp
80103691:	8d 83 34 02 00 00    	lea    0x234(%ebx),%eax
80103697:	50                   	push   %eax
80103698:	e8 43 0a 00 00       	call   801040e0 <wakeup>
  release(&p->lock);
8010369d:	89 1c 24             	mov    %ebx,(%esp)
801036a0:	e8 9b 0e 00 00       	call   80104540 <release>
  return n;
801036a5:	83 c4 10             	add    $0x10,%esp
801036a8:	89 f8                	mov    %edi,%eax
801036aa:	eb a5                	jmp    80103651 <pipewrite+0xb1>
    while(p->nwrite == p->nread + PIPESIZE){  //DOC: pipewrite-full
801036ac:	8b 55 e4             	mov    -0x1c(%ebp),%edx
801036af:	eb b2                	jmp    80103663 <pipewrite+0xc3>
801036b1:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801036b8:	00 
801036b9:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi

801036c0 <piperead>:

int
piperead(struct pipe *p, char *addr, int n)
{
801036c0:	55                   	push   %ebp
801036c1:	89 e5                	mov    %esp,%ebp
801036c3:	57                   	push   %edi
801036c4:	56                   	push   %esi
801036c5:	53                   	push   %ebx
801036c6:	83 ec 18             	sub    $0x18,%esp
801036c9:	8b 75 08             	mov    0x8(%ebp),%esi
801036cc:	8b 7d 0c             	mov    0xc(%ebp),%edi
  int i;

  acquire(&p->lock);
801036cf:	56                   	push   %esi
801036d0:	8d 9e 34 02 00 00    	lea    0x234(%esi),%ebx
801036d6:	e8 c5 0e 00 00       	call   801045a0 <acquire>
  while(p->nread == p->nwrite && p->writeopen){  //DOC: pipe-empty
801036db:	8b 86 34 02 00 00    	mov    0x234(%esi),%eax
801036e1:	83 c4 10             	add    $0x10,%esp
801036e4:	3b 86 38 02 00 00    	cmp    0x238(%esi),%eax
801036ea:	74 2f                	je     8010371b <piperead+0x5b>
801036ec:	eb 37                	jmp    80103725 <piperead+0x65>
801036ee:	66 90                	xchg   %ax,%ax
    if(myproc()->killed){
801036f0:	e8 6b 02 00 00       	call   80103960 <myproc>
801036f5:	8b 40 24             	mov    0x24(%eax),%eax
801036f8:	85 c0                	test   %eax,%eax
801036fa:	0f 85 80 00 00 00    	jne    80103780 <piperead+0xc0>
      release(&p->lock);
      return -1;
    }
    sleep(&p->nread, &p->lock); //DOC: piperead-sleep
80103700:	83 ec 08             	sub    $0x8,%esp
80103703:	56                   	push   %esi
80103704:	53                   	push   %ebx
80103705:	e8 16 09 00 00       	call   80104020 <sleep>
  while(p->nread == p->nwrite && p->writeopen){  //DOC: pipe-empty
8010370a:	8b 86 34 02 00 00    	mov    0x234(%esi),%eax
80103710:	83 c4 10             	add    $0x10,%esp
80103713:	3b 86 38 02 00 00    	cmp    0x238(%esi),%eax
80103719:	75 0a                	jne    80103725 <piperead+0x65>
8010371b:	8b 96 40 02 00 00    	mov    0x240(%esi),%edx
80103721:	85 d2                	test   %edx,%edx
80103723:	75 cb                	jne    801036f0 <piperead+0x30>
  }
  for(i = 0; i < n; i++){  //DOC: piperead-copy
80103725:	8b 4d 10             	mov    0x10(%ebp),%ecx
80103728:	31 db                	xor    %ebx,%ebx
8010372a:	85 c9                	test   %ecx,%ecx
8010372c:	7f 26                	jg     80103754 <piperead+0x94>
8010372e:	eb 2c                	jmp    8010375c <piperead+0x9c>
    if(p->nread == p->nwrite)
      break;
    addr[i] = p->data[p->nread++ % PIPESIZE];
80103730:	8d 48 01             	lea    0x1(%eax),%ecx
80103733:	25 ff 01 00 00       	and    $0x1ff,%eax
80103738:	89 8e 34 02 00 00    	mov    %ecx,0x234(%esi)
8010373e:	0f b6 44 06 34       	movzbl 0x34(%esi,%eax,1),%eax
80103743:	88 04 1f             	mov    %al,(%edi,%ebx,1)
  for(i = 0; i < n; i++){  //DOC: piperead-copy
80103746:	83 c3 01             	add    $0x1,%ebx
80103749:	39 5d 10             	cmp    %ebx,0x10(%ebp)
8010374c:	74 0e                	je     8010375c <piperead+0x9c>
8010374e:	8b 86 34 02 00 00    	mov    0x234(%esi),%eax
    if(p->nread == p->nwrite)
80103754:	3b 86 38 02 00 00    	cmp    0x238(%esi),%eax
8010375a:	75 d4                	jne    80103730 <piperead+0x70>
  }
  wakeup(&p->nwrite);  //DOC: piperead-wakeup
8010375c:	83 ec 0c             	sub    $0xc,%esp
8010375f:	8d 86 38 02 00 00    	lea    0x238(%esi),%eax
80103765:	50                   	push   %eax
80103766:	e8 75 09 00 00       	call   801040e0 <wakeup>
  release(&p->lock);
8010376b:	89 34 24             	mov    %esi,(%esp)
8010376e:	e8 cd 0d 00 00       	call   80104540 <release>
  return i;
80103773:	83 c4 10             	add    $0x10,%esp
}
80103776:	8d 65 f4             	lea    -0xc(%ebp),%esp
80103779:	89 d8                	mov    %ebx,%eax
8010377b:	5b                   	pop    %ebx
8010377c:	5e                   	pop    %esi
8010377d:	5f                   	pop    %edi
8010377e:	5d                   	pop    %ebp
8010377f:	c3                   	ret
      release(&p->lock);
80103780:	83 ec 0c             	sub    $0xc,%esp
      return -1;
80103783:	bb ff ff ff ff       	mov    $0xffffffff,%ebx
      release(&p->lock);
80103788:	56                   	push   %esi
80103789:	e8 b2 0d 00 00       	call   80104540 <release>
      return -1;
8010378e:	83 c4 10             	add    $0x10,%esp
}
80103791:	8d 65 f4             	lea    -0xc(%ebp),%esp
80103794:	89 d8                	mov    %ebx,%eax
80103796:	5b                   	pop    %ebx
80103797:	5e                   	pop    %esi
80103798:	5f                   	pop    %edi
80103799:	5d                   	pop    %ebp
8010379a:	c3                   	ret
8010379b:	66 90                	xchg   %ax,%ax
8010379d:	66 90                	xchg   %ax,%ax
8010379f:	90                   	nop

801037a0 <allocproc>:
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
801037a0:	55                   	push   %ebp
801037a1:	89 e5                	mov    %esp,%ebp
801037a3:	53                   	push   %ebx
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
801037a4:	bb 54 1d 11 80       	mov    $0x80111d54,%ebx
{
801037a9:	83 ec 10             	sub    $0x10,%esp
  acquire(&ptable.lock);
801037ac:	68 20 1d 11 80       	push   $0x80111d20
801037b1:	e8 ea 0d 00 00       	call   801045a0 <acquire>
801037b6:	83 c4 10             	add    $0x10,%esp
801037b9:	eb 10                	jmp    801037cb <allocproc+0x2b>
801037bb:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
801037c0:	83 c3 7c             	add    $0x7c,%ebx
801037c3:	81 fb 54 3c 11 80    	cmp    $0x80113c54,%ebx
801037c9:	74 75                	je     80103840 <allocproc+0xa0>
    if(p->state == UNUSED)
801037cb:	8b 43 0c             	mov    0xc(%ebx),%eax
801037ce:	85 c0                	test   %eax,%eax
801037d0:	75 ee                	jne    801037c0 <allocproc+0x20>
  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
801037d2:	a1 04 a0 10 80       	mov    0x8010a004,%eax

  release(&ptable.lock);
801037d7:	83 ec 0c             	sub    $0xc,%esp
  p->state = EMBRYO;
801037da:	c7 43 0c 01 00 00 00 	movl   $0x1,0xc(%ebx)
  p->pid = nextpid++;
801037e1:	89 43 10             	mov    %eax,0x10(%ebx)
801037e4:	8d 50 01             	lea    0x1(%eax),%edx
  release(&ptable.lock);
801037e7:	68 20 1d 11 80       	push   $0x80111d20
  p->pid = nextpid++;
801037ec:	89 15 04 a0 10 80    	mov    %edx,0x8010a004
  release(&ptable.lock);
801037f2:	e8 49 0d 00 00       	call   80104540 <release>

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
801037f7:	e8 64 ee ff ff       	call   80102660 <kalloc>
801037fc:	83 c4 10             	add    $0x10,%esp
801037ff:	89 43 08             	mov    %eax,0x8(%ebx)
80103802:	85 c0                	test   %eax,%eax
80103804:	74 53                	je     80103859 <allocproc+0xb9>
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
80103806:	8d 90 b4 0f 00 00    	lea    0xfb4(%eax),%edx
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
8010380c:	83 ec 04             	sub    $0x4,%esp
  sp -= sizeof *p->context;
8010380f:	05 9c 0f 00 00       	add    $0xf9c,%eax
  sp -= sizeof *p->tf;
80103814:	89 53 18             	mov    %edx,0x18(%ebx)
  *(uint*)sp = (uint)trapret;
80103817:	c7 40 14 52 58 10 80 	movl   $0x80105852,0x14(%eax)
  p->context = (struct context*)sp;
8010381e:	89 43 1c             	mov    %eax,0x1c(%ebx)
  memset(p->context, 0, sizeof *p->context);
80103821:	6a 14                	push   $0x14
80103823:	6a 00                	push   $0x0
80103825:	50                   	push   %eax
80103826:	e8 75 0e 00 00       	call   801046a0 <memset>
  p->context->eip = (uint)forkret;
8010382b:	8b 43 1c             	mov    0x1c(%ebx),%eax

  return p;
8010382e:	83 c4 10             	add    $0x10,%esp
  p->context->eip = (uint)forkret;
80103831:	c7 40 10 70 38 10 80 	movl   $0x80103870,0x10(%eax)
}
80103838:	89 d8                	mov    %ebx,%eax
8010383a:	8b 5d fc             	mov    -0x4(%ebp),%ebx
8010383d:	c9                   	leave
8010383e:	c3                   	ret
8010383f:	90                   	nop
  release(&ptable.lock);
80103840:	83 ec 0c             	sub    $0xc,%esp
  return 0;
80103843:	31 db                	xor    %ebx,%ebx
  release(&ptable.lock);
80103845:	68 20 1d 11 80       	push   $0x80111d20
8010384a:	e8 f1 0c 00 00       	call   80104540 <release>
  return 0;
8010384f:	83 c4 10             	add    $0x10,%esp
}
80103852:	89 d8                	mov    %ebx,%eax
80103854:	8b 5d fc             	mov    -0x4(%ebp),%ebx
80103857:	c9                   	leave
80103858:	c3                   	ret
    p->state = UNUSED;
80103859:	c7 43 0c 00 00 00 00 	movl   $0x0,0xc(%ebx)
  return 0;
80103860:	31 db                	xor    %ebx,%ebx
80103862:	eb ee                	jmp    80103852 <allocproc+0xb2>
80103864:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010386b:	00 
8010386c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi

80103870 <forkret>:

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
80103870:	55                   	push   %ebp
80103871:	89 e5                	mov    %esp,%ebp
80103873:	83 ec 14             	sub    $0x14,%esp
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);
80103876:	68 20 1d 11 80       	push   $0x80111d20
8010387b:	e8 c0 0c 00 00       	call   80104540 <release>

  if (first) {
80103880:	a1 00 a0 10 80       	mov    0x8010a000,%eax
80103885:	83 c4 10             	add    $0x10,%esp
80103888:	85 c0                	test   %eax,%eax
8010388a:	75 04                	jne    80103890 <forkret+0x20>
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}
8010388c:	c9                   	leave
8010388d:	c3                   	ret
8010388e:	66 90                	xchg   %ax,%ax
    first = 0;
80103890:	c7 05 00 a0 10 80 00 	movl   $0x0,0x8010a000
80103897:	00 00 00 
    iinit(ROOTDEV);
8010389a:	83 ec 0c             	sub    $0xc,%esp
8010389d:	6a 01                	push   $0x1
8010389f:	e8 dc dc ff ff       	call   80101580 <iinit>
    initlog(ROOTDEV);
801038a4:	c7 04 24 01 00 00 00 	movl   $0x1,(%esp)
801038ab:	e8 f0 f3 ff ff       	call   80102ca0 <initlog>
}
801038b0:	83 c4 10             	add    $0x10,%esp
801038b3:	c9                   	leave
801038b4:	c3                   	ret
801038b5:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801038bc:	00 
801038bd:	8d 76 00             	lea    0x0(%esi),%esi

801038c0 <pinit>:
{
801038c0:	55                   	push   %ebp
801038c1:	89 e5                	mov    %esp,%ebp
801038c3:	83 ec 10             	sub    $0x10,%esp
  initlock(&ptable.lock, "ptable");
801038c6:	68 b6 73 10 80       	push   $0x801073b6
801038cb:	68 20 1d 11 80       	push   $0x80111d20
801038d0:	e8 db 0a 00 00       	call   801043b0 <initlock>
}
801038d5:	83 c4 10             	add    $0x10,%esp
801038d8:	c9                   	leave
801038d9:	c3                   	ret
801038da:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi

801038e0 <mycpu>:
{
801038e0:	55                   	push   %ebp
801038e1:	89 e5                	mov    %esp,%ebp
801038e3:	56                   	push   %esi
801038e4:	53                   	push   %ebx
  asm volatile("pushfl; popl %0" : "=r" (eflags));
801038e5:	9c                   	pushf
801038e6:	58                   	pop    %eax
  if(readeflags()&FL_IF)
801038e7:	f6 c4 02             	test   $0x2,%ah
801038ea:	75 46                	jne    80103932 <mycpu+0x52>
  apicid = lapicid();
801038ec:	e8 df ef ff ff       	call   801028d0 <lapicid>
  for (i = 0; i < ncpu; ++i) {
801038f1:	8b 35 84 17 11 80    	mov    0x80111784,%esi
801038f7:	85 f6                	test   %esi,%esi
801038f9:	7e 2a                	jle    80103925 <mycpu+0x45>
801038fb:	31 d2                	xor    %edx,%edx
801038fd:	eb 08                	jmp    80103907 <mycpu+0x27>
801038ff:	90                   	nop
80103900:	83 c2 01             	add    $0x1,%edx
80103903:	39 f2                	cmp    %esi,%edx
80103905:	74 1e                	je     80103925 <mycpu+0x45>
    if (cpus[i].apicid == apicid)
80103907:	69 ca b0 00 00 00    	imul   $0xb0,%edx,%ecx
8010390d:	0f b6 99 a0 17 11 80 	movzbl -0x7feee860(%ecx),%ebx
80103914:	39 c3                	cmp    %eax,%ebx
80103916:	75 e8                	jne    80103900 <mycpu+0x20>
}
80103918:	8d 65 f8             	lea    -0x8(%ebp),%esp
      return &cpus[i];
8010391b:	8d 81 a0 17 11 80    	lea    -0x7feee860(%ecx),%eax
}
80103921:	5b                   	pop    %ebx
80103922:	5e                   	pop    %esi
80103923:	5d                   	pop    %ebp
80103924:	c3                   	ret
  panic("unknown apicid\n");
80103925:	83 ec 0c             	sub    $0xc,%esp
80103928:	68 bd 73 10 80       	push   $0x801073bd
8010392d:	e8 4e ca ff ff       	call   80100380 <panic>
    panic("mycpu called with interrupts enabled\n");
80103932:	83 ec 0c             	sub    $0xc,%esp
80103935:	68 1c 77 10 80       	push   $0x8010771c
8010393a:	e8 41 ca ff ff       	call   80100380 <panic>
8010393f:	90                   	nop

80103940 <cpuid>:
cpuid() {
80103940:	55                   	push   %ebp
80103941:	89 e5                	mov    %esp,%ebp
80103943:	83 ec 08             	sub    $0x8,%esp
  return mycpu()-cpus;
80103946:	e8 95 ff ff ff       	call   801038e0 <mycpu>
}
8010394b:	c9                   	leave
  return mycpu()-cpus;
8010394c:	2d a0 17 11 80       	sub    $0x801117a0,%eax
80103951:	c1 f8 04             	sar    $0x4,%eax
80103954:	69 c0 a3 8b 2e ba    	imul   $0xba2e8ba3,%eax,%eax
}
8010395a:	c3                   	ret
8010395b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi

80103960 <myproc>:
myproc(void) {
80103960:	55                   	push   %ebp
80103961:	89 e5                	mov    %esp,%ebp
80103963:	53                   	push   %ebx
80103964:	83 ec 04             	sub    $0x4,%esp
  pushcli();
80103967:	e8 e4 0a 00 00       	call   80104450 <pushcli>
  c = mycpu();
8010396c:	e8 6f ff ff ff       	call   801038e0 <mycpu>
  p = c->proc;
80103971:	8b 98 ac 00 00 00    	mov    0xac(%eax),%ebx
  popcli();
80103977:	e8 24 0b 00 00       	call   801044a0 <popcli>
}
8010397c:	89 d8                	mov    %ebx,%eax
8010397e:	8b 5d fc             	mov    -0x4(%ebp),%ebx
80103981:	c9                   	leave
80103982:	c3                   	ret
80103983:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010398a:	00 
8010398b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi

80103990 <userinit>:
{
80103990:	55                   	push   %ebp
80103991:	89 e5                	mov    %esp,%ebp
80103993:	53                   	push   %ebx
80103994:	83 ec 04             	sub    $0x4,%esp
  p = allocproc();
80103997:	e8 04 fe ff ff       	call   801037a0 <allocproc>
8010399c:	89 c3                	mov    %eax,%ebx
  initproc = p;
8010399e:	a3 54 3c 11 80       	mov    %eax,0x80113c54
  if((p->pgdir = setupkvm()) == 0)
801039a3:	e8 78 34 00 00       	call   80106e20 <setupkvm>
801039a8:	89 43 04             	mov    %eax,0x4(%ebx)
801039ab:	85 c0                	test   %eax,%eax
801039ad:	0f 84 bd 00 00 00    	je     80103a70 <userinit+0xe0>
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
801039b3:	83 ec 04             	sub    $0x4,%esp
801039b6:	68 2c 00 00 00       	push   $0x2c
801039bb:	68 60 a4 10 80       	push   $0x8010a460
801039c0:	50                   	push   %eax
801039c1:	e8 3a 31 00 00       	call   80106b00 <inituvm>
  memset(p->tf, 0, sizeof(*p->tf));
801039c6:	83 c4 0c             	add    $0xc,%esp
  p->sz = PGSIZE;
801039c9:	c7 03 00 10 00 00    	movl   $0x1000,(%ebx)
  memset(p->tf, 0, sizeof(*p->tf));
801039cf:	6a 4c                	push   $0x4c
801039d1:	6a 00                	push   $0x0
801039d3:	ff 73 18             	push   0x18(%ebx)
801039d6:	e8 c5 0c 00 00       	call   801046a0 <memset>
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
801039db:	8b 43 18             	mov    0x18(%ebx),%eax
801039de:	ba 1b 00 00 00       	mov    $0x1b,%edx
  safestrcpy(p->name, "initcode", sizeof(p->name));
801039e3:	83 c4 0c             	add    $0xc,%esp
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
801039e6:	b9 23 00 00 00       	mov    $0x23,%ecx
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
801039eb:	66 89 50 3c          	mov    %dx,0x3c(%eax)
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
801039ef:	8b 43 18             	mov    0x18(%ebx),%eax
801039f2:	66 89 48 2c          	mov    %cx,0x2c(%eax)
  p->tf->es = p->tf->ds;
801039f6:	8b 43 18             	mov    0x18(%ebx),%eax
801039f9:	0f b7 50 2c          	movzwl 0x2c(%eax),%edx
801039fd:	66 89 50 28          	mov    %dx,0x28(%eax)
  p->tf->ss = p->tf->ds;
80103a01:	8b 43 18             	mov    0x18(%ebx),%eax
80103a04:	0f b7 50 2c          	movzwl 0x2c(%eax),%edx
80103a08:	66 89 50 48          	mov    %dx,0x48(%eax)
  p->tf->eflags = FL_IF;
80103a0c:	8b 43 18             	mov    0x18(%ebx),%eax
80103a0f:	c7 40 40 00 02 00 00 	movl   $0x200,0x40(%eax)
  p->tf->esp = PGSIZE;
80103a16:	8b 43 18             	mov    0x18(%ebx),%eax
80103a19:	c7 40 44 00 10 00 00 	movl   $0x1000,0x44(%eax)
  p->tf->eip = 0;  // beginning of initcode.S
80103a20:	8b 43 18             	mov    0x18(%ebx),%eax
80103a23:	c7 40 38 00 00 00 00 	movl   $0x0,0x38(%eax)
  safestrcpy(p->name, "initcode", sizeof(p->name));
80103a2a:	8d 43 6c             	lea    0x6c(%ebx),%eax
80103a2d:	6a 10                	push   $0x10
80103a2f:	68 e6 73 10 80       	push   $0x801073e6
80103a34:	50                   	push   %eax
80103a35:	e8 16 0e 00 00       	call   80104850 <safestrcpy>
  p->cwd = namei("/");
80103a3a:	c7 04 24 ef 73 10 80 	movl   $0x801073ef,(%esp)
80103a41:	e8 3a e6 ff ff       	call   80102080 <namei>
80103a46:	89 43 68             	mov    %eax,0x68(%ebx)
  acquire(&ptable.lock);
80103a49:	c7 04 24 20 1d 11 80 	movl   $0x80111d20,(%esp)
80103a50:	e8 4b 0b 00 00       	call   801045a0 <acquire>
  p->state = RUNNABLE;
80103a55:	c7 43 0c 03 00 00 00 	movl   $0x3,0xc(%ebx)
  release(&ptable.lock);
80103a5c:	c7 04 24 20 1d 11 80 	movl   $0x80111d20,(%esp)
80103a63:	e8 d8 0a 00 00       	call   80104540 <release>
}
80103a68:	8b 5d fc             	mov    -0x4(%ebp),%ebx
80103a6b:	83 c4 10             	add    $0x10,%esp
80103a6e:	c9                   	leave
80103a6f:	c3                   	ret
    panic("userinit: out of memory?");
80103a70:	83 ec 0c             	sub    $0xc,%esp
80103a73:	68 cd 73 10 80       	push   $0x801073cd
80103a78:	e8 03 c9 ff ff       	call   80100380 <panic>
80103a7d:	8d 76 00             	lea    0x0(%esi),%esi

80103a80 <growproc>:
{
80103a80:	55                   	push   %ebp
80103a81:	89 e5                	mov    %esp,%ebp
80103a83:	56                   	push   %esi
80103a84:	53                   	push   %ebx
80103a85:	8b 75 08             	mov    0x8(%ebp),%esi
  pushcli();
80103a88:	e8 c3 09 00 00       	call   80104450 <pushcli>
  c = mycpu();
80103a8d:	e8 4e fe ff ff       	call   801038e0 <mycpu>
  p = c->proc;
80103a92:	8b 98 ac 00 00 00    	mov    0xac(%eax),%ebx
  popcli();
80103a98:	e8 03 0a 00 00       	call   801044a0 <popcli>
  sz = curproc->sz;
80103a9d:	8b 03                	mov    (%ebx),%eax
  if(n > 0){
80103a9f:	85 f6                	test   %esi,%esi
80103aa1:	7f 1d                	jg     80103ac0 <growproc+0x40>
  } else if(n < 0){
80103aa3:	75 3b                	jne    80103ae0 <growproc+0x60>
  switchuvm(curproc);
80103aa5:	83 ec 0c             	sub    $0xc,%esp
  curproc->sz = sz;
80103aa8:	89 03                	mov    %eax,(%ebx)
  switchuvm(curproc);
80103aaa:	53                   	push   %ebx
80103aab:	e8 40 2f 00 00       	call   801069f0 <switchuvm>
  return 0;
80103ab0:	83 c4 10             	add    $0x10,%esp
80103ab3:	31 c0                	xor    %eax,%eax
}
80103ab5:	8d 65 f8             	lea    -0x8(%ebp),%esp
80103ab8:	5b                   	pop    %ebx
80103ab9:	5e                   	pop    %esi
80103aba:	5d                   	pop    %ebp
80103abb:	c3                   	ret
80103abc:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
80103ac0:	83 ec 04             	sub    $0x4,%esp
80103ac3:	01 c6                	add    %eax,%esi
80103ac5:	56                   	push   %esi
80103ac6:	50                   	push   %eax
80103ac7:	ff 73 04             	push   0x4(%ebx)
80103aca:	e8 81 31 00 00       	call   80106c50 <allocuvm>
80103acf:	83 c4 10             	add    $0x10,%esp
80103ad2:	85 c0                	test   %eax,%eax
80103ad4:	75 cf                	jne    80103aa5 <growproc+0x25>
      return -1;
80103ad6:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
80103adb:	eb d8                	jmp    80103ab5 <growproc+0x35>
80103add:	8d 76 00             	lea    0x0(%esi),%esi
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
80103ae0:	83 ec 04             	sub    $0x4,%esp
80103ae3:	01 c6                	add    %eax,%esi
80103ae5:	56                   	push   %esi
80103ae6:	50                   	push   %eax
80103ae7:	ff 73 04             	push   0x4(%ebx)
80103aea:	e8 81 32 00 00       	call   80106d70 <deallocuvm>
80103aef:	83 c4 10             	add    $0x10,%esp
80103af2:	85 c0                	test   %eax,%eax
80103af4:	75 af                	jne    80103aa5 <growproc+0x25>
80103af6:	eb de                	jmp    80103ad6 <growproc+0x56>
80103af8:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80103aff:	00 

80103b00 <fork>:
{
80103b00:	55                   	push   %ebp
80103b01:	89 e5                	mov    %esp,%ebp
80103b03:	57                   	push   %edi
80103b04:	56                   	push   %esi
80103b05:	53                   	push   %ebx
80103b06:	83 ec 1c             	sub    $0x1c,%esp
  pushcli();
80103b09:	e8 42 09 00 00       	call   80104450 <pushcli>
  c = mycpu();
80103b0e:	e8 cd fd ff ff       	call   801038e0 <mycpu>
  p = c->proc;
80103b13:	8b 98 ac 00 00 00    	mov    0xac(%eax),%ebx
  popcli();
80103b19:	e8 82 09 00 00       	call   801044a0 <popcli>
  if((np = allocproc()) == 0){
80103b1e:	e8 7d fc ff ff       	call   801037a0 <allocproc>
80103b23:	89 45 e4             	mov    %eax,-0x1c(%ebp)
80103b26:	85 c0                	test   %eax,%eax
80103b28:	0f 84 d6 00 00 00    	je     80103c04 <fork+0x104>
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
80103b2e:	83 ec 08             	sub    $0x8,%esp
80103b31:	ff 33                	push   (%ebx)
80103b33:	89 c7                	mov    %eax,%edi
80103b35:	ff 73 04             	push   0x4(%ebx)
80103b38:	e8 d3 33 00 00       	call   80106f10 <copyuvm>
80103b3d:	83 c4 10             	add    $0x10,%esp
80103b40:	89 47 04             	mov    %eax,0x4(%edi)
80103b43:	85 c0                	test   %eax,%eax
80103b45:	0f 84 9a 00 00 00    	je     80103be5 <fork+0xe5>
  np->sz = curproc->sz;
80103b4b:	8b 03                	mov    (%ebx),%eax
80103b4d:	8b 4d e4             	mov    -0x1c(%ebp),%ecx
80103b50:	89 01                	mov    %eax,(%ecx)
  *np->tf = *curproc->tf;
80103b52:	8b 79 18             	mov    0x18(%ecx),%edi
  np->parent = curproc;
80103b55:	89 c8                	mov    %ecx,%eax
80103b57:	89 59 14             	mov    %ebx,0x14(%ecx)
  *np->tf = *curproc->tf;
80103b5a:	b9 13 00 00 00       	mov    $0x13,%ecx
80103b5f:	8b 73 18             	mov    0x18(%ebx),%esi
80103b62:	f3 a5                	rep movsl %ds:(%esi),%es:(%edi)
  for(i = 0; i < NOFILE; i++)
80103b64:	31 f6                	xor    %esi,%esi
  np->tf->eax = 0;
80103b66:	8b 40 18             	mov    0x18(%eax),%eax
80103b69:	c7 40 1c 00 00 00 00 	movl   $0x0,0x1c(%eax)
    if(curproc->ofile[i])
80103b70:	8b 44 b3 28          	mov    0x28(%ebx,%esi,4),%eax
80103b74:	85 c0                	test   %eax,%eax
80103b76:	74 13                	je     80103b8b <fork+0x8b>
      np->ofile[i] = filedup(curproc->ofile[i]);
80103b78:	83 ec 0c             	sub    $0xc,%esp
80103b7b:	50                   	push   %eax
80103b7c:	e8 3f d3 ff ff       	call   80100ec0 <filedup>
80103b81:	8b 55 e4             	mov    -0x1c(%ebp),%edx
80103b84:	83 c4 10             	add    $0x10,%esp
80103b87:	89 44 b2 28          	mov    %eax,0x28(%edx,%esi,4)
  for(i = 0; i < NOFILE; i++)
80103b8b:	83 c6 01             	add    $0x1,%esi
80103b8e:	83 fe 10             	cmp    $0x10,%esi
80103b91:	75 dd                	jne    80103b70 <fork+0x70>
  np->cwd = idup(curproc->cwd);
80103b93:	83 ec 0c             	sub    $0xc,%esp
80103b96:	ff 73 68             	push   0x68(%ebx)
  safestrcpy(np->name, curproc->name, sizeof(curproc->name));
80103b99:	83 c3 6c             	add    $0x6c,%ebx
  np->cwd = idup(curproc->cwd);
80103b9c:	e8 cf db ff ff       	call   80101770 <idup>
80103ba1:	8b 7d e4             	mov    -0x1c(%ebp),%edi
  safestrcpy(np->name, curproc->name, sizeof(curproc->name));
80103ba4:	83 c4 0c             	add    $0xc,%esp
  np->cwd = idup(curproc->cwd);
80103ba7:	89 47 68             	mov    %eax,0x68(%edi)
  safestrcpy(np->name, curproc->name, sizeof(curproc->name));
80103baa:	8d 47 6c             	lea    0x6c(%edi),%eax
80103bad:	6a 10                	push   $0x10
80103baf:	53                   	push   %ebx
80103bb0:	50                   	push   %eax
80103bb1:	e8 9a 0c 00 00       	call   80104850 <safestrcpy>
  pid = np->pid;
80103bb6:	8b 5f 10             	mov    0x10(%edi),%ebx
  acquire(&ptable.lock);
80103bb9:	c7 04 24 20 1d 11 80 	movl   $0x80111d20,(%esp)
80103bc0:	e8 db 09 00 00       	call   801045a0 <acquire>
  np->state = RUNNABLE;
80103bc5:	c7 47 0c 03 00 00 00 	movl   $0x3,0xc(%edi)
  release(&ptable.lock);
80103bcc:	c7 04 24 20 1d 11 80 	movl   $0x80111d20,(%esp)
80103bd3:	e8 68 09 00 00       	call   80104540 <release>
  return pid;
80103bd8:	83 c4 10             	add    $0x10,%esp
}
80103bdb:	8d 65 f4             	lea    -0xc(%ebp),%esp
80103bde:	89 d8                	mov    %ebx,%eax
80103be0:	5b                   	pop    %ebx
80103be1:	5e                   	pop    %esi
80103be2:	5f                   	pop    %edi
80103be3:	5d                   	pop    %ebp
80103be4:	c3                   	ret
    kfree(np->kstack);
80103be5:	8b 5d e4             	mov    -0x1c(%ebp),%ebx
80103be8:	83 ec 0c             	sub    $0xc,%esp
80103beb:	ff 73 08             	push   0x8(%ebx)
80103bee:	e8 ad e8 ff ff       	call   801024a0 <kfree>
    np->kstack = 0;
80103bf3:	c7 43 08 00 00 00 00 	movl   $0x0,0x8(%ebx)
    return -1;
80103bfa:	83 c4 10             	add    $0x10,%esp
    np->state = UNUSED;
80103bfd:	c7 43 0c 00 00 00 00 	movl   $0x0,0xc(%ebx)
    return -1;
80103c04:	bb ff ff ff ff       	mov    $0xffffffff,%ebx
80103c09:	eb d0                	jmp    80103bdb <fork+0xdb>
80103c0b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi

80103c10 <scheduler>:
{
80103c10:	55                   	push   %ebp
80103c11:	89 e5                	mov    %esp,%ebp
80103c13:	57                   	push   %edi
80103c14:	56                   	push   %esi
80103c15:	53                   	push   %ebx
80103c16:	83 ec 0c             	sub    $0xc,%esp
  struct cpu *c = mycpu();
80103c19:	e8 c2 fc ff ff       	call   801038e0 <mycpu>
  c->proc = 0;
80103c1e:	c7 80 ac 00 00 00 00 	movl   $0x0,0xac(%eax)
80103c25:	00 00 00 
  struct cpu *c = mycpu();
80103c28:	89 c6                	mov    %eax,%esi
  c->proc = 0;
80103c2a:	8d 78 04             	lea    0x4(%eax),%edi
80103c2d:	8d 76 00             	lea    0x0(%esi),%esi
  asm volatile("sti");
80103c30:	fb                   	sti
    acquire(&ptable.lock);
80103c31:	83 ec 0c             	sub    $0xc,%esp
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
80103c34:	bb 54 1d 11 80       	mov    $0x80111d54,%ebx
    acquire(&ptable.lock);
80103c39:	68 20 1d 11 80       	push   $0x80111d20
80103c3e:	e8 5d 09 00 00       	call   801045a0 <acquire>
80103c43:	83 c4 10             	add    $0x10,%esp
80103c46:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80103c4d:	00 
80103c4e:	66 90                	xchg   %ax,%ax
      if(p->state != RUNNABLE)
80103c50:	83 7b 0c 03          	cmpl   $0x3,0xc(%ebx)
80103c54:	75 33                	jne    80103c89 <scheduler+0x79>
      switchuvm(p);
80103c56:	83 ec 0c             	sub    $0xc,%esp
      c->proc = p;
80103c59:	89 9e ac 00 00 00    	mov    %ebx,0xac(%esi)
      switchuvm(p);
80103c5f:	53                   	push   %ebx
80103c60:	e8 8b 2d 00 00       	call   801069f0 <switchuvm>
      swtch(&(c->scheduler), p->context);
80103c65:	58                   	pop    %eax
80103c66:	5a                   	pop    %edx
80103c67:	ff 73 1c             	push   0x1c(%ebx)
80103c6a:	57                   	push   %edi
      p->state = RUNNING;
80103c6b:	c7 43 0c 04 00 00 00 	movl   $0x4,0xc(%ebx)
      swtch(&(c->scheduler), p->context);
80103c72:	e8 34 0c 00 00       	call   801048ab <swtch>
      switchkvm();
80103c77:	e8 64 2d 00 00       	call   801069e0 <switchkvm>
      c->proc = 0;
80103c7c:	83 c4 10             	add    $0x10,%esp
80103c7f:	c7 86 ac 00 00 00 00 	movl   $0x0,0xac(%esi)
80103c86:	00 00 00 
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
80103c89:	83 c3 7c             	add    $0x7c,%ebx
80103c8c:	81 fb 54 3c 11 80    	cmp    $0x80113c54,%ebx
80103c92:	75 bc                	jne    80103c50 <scheduler+0x40>
    release(&ptable.lock);
80103c94:	83 ec 0c             	sub    $0xc,%esp
80103c97:	68 20 1d 11 80       	push   $0x80111d20
80103c9c:	e8 9f 08 00 00       	call   80104540 <release>
    sti();
80103ca1:	83 c4 10             	add    $0x10,%esp
80103ca4:	eb 8a                	jmp    80103c30 <scheduler+0x20>
80103ca6:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80103cad:	00 
80103cae:	66 90                	xchg   %ax,%ax

80103cb0 <sched>:
{
80103cb0:	55                   	push   %ebp
80103cb1:	89 e5                	mov    %esp,%ebp
80103cb3:	56                   	push   %esi
80103cb4:	53                   	push   %ebx
  pushcli();
80103cb5:	e8 96 07 00 00       	call   80104450 <pushcli>
  c = mycpu();
80103cba:	e8 21 fc ff ff       	call   801038e0 <mycpu>
  p = c->proc;
80103cbf:	8b 98 ac 00 00 00    	mov    0xac(%eax),%ebx
  popcli();
80103cc5:	e8 d6 07 00 00       	call   801044a0 <popcli>
  if(!holding(&ptable.lock))
80103cca:	83 ec 0c             	sub    $0xc,%esp
80103ccd:	68 20 1d 11 80       	push   $0x80111d20
80103cd2:	e8 29 08 00 00       	call   80104500 <holding>
80103cd7:	83 c4 10             	add    $0x10,%esp
80103cda:	85 c0                	test   %eax,%eax
80103cdc:	74 4f                	je     80103d2d <sched+0x7d>
  if(mycpu()->ncli != 1)
80103cde:	e8 fd fb ff ff       	call   801038e0 <mycpu>
80103ce3:	83 b8 a4 00 00 00 01 	cmpl   $0x1,0xa4(%eax)
80103cea:	75 68                	jne    80103d54 <sched+0xa4>
  if(p->state == RUNNING)
80103cec:	83 7b 0c 04          	cmpl   $0x4,0xc(%ebx)
80103cf0:	74 55                	je     80103d47 <sched+0x97>
  asm volatile("pushfl; popl %0" : "=r" (eflags));
80103cf2:	9c                   	pushf
80103cf3:	58                   	pop    %eax
  if(readeflags()&FL_IF)
80103cf4:	f6 c4 02             	test   $0x2,%ah
80103cf7:	75 41                	jne    80103d3a <sched+0x8a>
  intena = mycpu()->intena;
80103cf9:	e8 e2 fb ff ff       	call   801038e0 <mycpu>
  swtch(&p->context, mycpu()->scheduler);
80103cfe:	83 c3 1c             	add    $0x1c,%ebx
  intena = mycpu()->intena;
80103d01:	8b b0 a8 00 00 00    	mov    0xa8(%eax),%esi
  swtch(&p->context, mycpu()->scheduler);
80103d07:	e8 d4 fb ff ff       	call   801038e0 <mycpu>
80103d0c:	83 ec 08             	sub    $0x8,%esp
80103d0f:	ff 70 04             	push   0x4(%eax)
80103d12:	53                   	push   %ebx
80103d13:	e8 93 0b 00 00       	call   801048ab <swtch>
  mycpu()->intena = intena;
80103d18:	e8 c3 fb ff ff       	call   801038e0 <mycpu>
}
80103d1d:	83 c4 10             	add    $0x10,%esp
  mycpu()->intena = intena;
80103d20:	89 b0 a8 00 00 00    	mov    %esi,0xa8(%eax)
}
80103d26:	8d 65 f8             	lea    -0x8(%ebp),%esp
80103d29:	5b                   	pop    %ebx
80103d2a:	5e                   	pop    %esi
80103d2b:	5d                   	pop    %ebp
80103d2c:	c3                   	ret
    panic("sched ptable.lock");
80103d2d:	83 ec 0c             	sub    $0xc,%esp
80103d30:	68 f1 73 10 80       	push   $0x801073f1
80103d35:	e8 46 c6 ff ff       	call   80100380 <panic>
    panic("sched interruptible");
80103d3a:	83 ec 0c             	sub    $0xc,%esp
80103d3d:	68 1d 74 10 80       	push   $0x8010741d
80103d42:	e8 39 c6 ff ff       	call   80100380 <panic>
    panic("sched running");
80103d47:	83 ec 0c             	sub    $0xc,%esp
80103d4a:	68 0f 74 10 80       	push   $0x8010740f
80103d4f:	e8 2c c6 ff ff       	call   80100380 <panic>
    panic("sched locks");
80103d54:	83 ec 0c             	sub    $0xc,%esp
80103d57:	68 03 74 10 80       	push   $0x80107403
80103d5c:	e8 1f c6 ff ff       	call   80100380 <panic>
80103d61:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80103d68:	00 
80103d69:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi

80103d70 <exit>:
{
80103d70:	55                   	push   %ebp
80103d71:	89 e5                	mov    %esp,%ebp
80103d73:	57                   	push   %edi
80103d74:	56                   	push   %esi
80103d75:	53                   	push   %ebx
80103d76:	83 ec 0c             	sub    $0xc,%esp
  struct proc *curproc = myproc();
80103d79:	e8 e2 fb ff ff       	call   80103960 <myproc>
  if(curproc == initproc)
80103d7e:	39 05 54 3c 11 80    	cmp    %eax,0x80113c54
80103d84:	0f 84 fd 00 00 00    	je     80103e87 <exit+0x117>
80103d8a:	89 c3                	mov    %eax,%ebx
80103d8c:	8d 70 28             	lea    0x28(%eax),%esi
80103d8f:	8d 78 68             	lea    0x68(%eax),%edi
80103d92:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    if(curproc->ofile[fd]){
80103d98:	8b 06                	mov    (%esi),%eax
80103d9a:	85 c0                	test   %eax,%eax
80103d9c:	74 12                	je     80103db0 <exit+0x40>
      fileclose(curproc->ofile[fd]);
80103d9e:	83 ec 0c             	sub    $0xc,%esp
80103da1:	50                   	push   %eax
80103da2:	e8 69 d1 ff ff       	call   80100f10 <fileclose>
      curproc->ofile[fd] = 0;
80103da7:	c7 06 00 00 00 00    	movl   $0x0,(%esi)
80103dad:	83 c4 10             	add    $0x10,%esp
  for(fd = 0; fd < NOFILE; fd++){
80103db0:	83 c6 04             	add    $0x4,%esi
80103db3:	39 f7                	cmp    %esi,%edi
80103db5:	75 e1                	jne    80103d98 <exit+0x28>
  begin_op();
80103db7:	e8 84 ef ff ff       	call   80102d40 <begin_op>
  iput(curproc->cwd);
80103dbc:	83 ec 0c             	sub    $0xc,%esp
80103dbf:	ff 73 68             	push   0x68(%ebx)
80103dc2:	e8 09 db ff ff       	call   801018d0 <iput>
  end_op();
80103dc7:	e8 e4 ef ff ff       	call   80102db0 <end_op>
  curproc->cwd = 0;
80103dcc:	c7 43 68 00 00 00 00 	movl   $0x0,0x68(%ebx)
  acquire(&ptable.lock);
80103dd3:	c7 04 24 20 1d 11 80 	movl   $0x80111d20,(%esp)
80103dda:	e8 c1 07 00 00       	call   801045a0 <acquire>
  wakeup1(curproc->parent);
80103ddf:	8b 53 14             	mov    0x14(%ebx),%edx
80103de2:	83 c4 10             	add    $0x10,%esp
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
80103de5:	b8 54 1d 11 80       	mov    $0x80111d54,%eax
80103dea:	eb 0e                	jmp    80103dfa <exit+0x8a>
80103dec:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
80103df0:	83 c0 7c             	add    $0x7c,%eax
80103df3:	3d 54 3c 11 80       	cmp    $0x80113c54,%eax
80103df8:	74 1c                	je     80103e16 <exit+0xa6>
    if(p->state == SLEEPING && p->chan == chan)
80103dfa:	83 78 0c 02          	cmpl   $0x2,0xc(%eax)
80103dfe:	75 f0                	jne    80103df0 <exit+0x80>
80103e00:	3b 50 20             	cmp    0x20(%eax),%edx
80103e03:	75 eb                	jne    80103df0 <exit+0x80>
      p->state = RUNNABLE;
80103e05:	c7 40 0c 03 00 00 00 	movl   $0x3,0xc(%eax)
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
80103e0c:	83 c0 7c             	add    $0x7c,%eax
80103e0f:	3d 54 3c 11 80       	cmp    $0x80113c54,%eax
80103e14:	75 e4                	jne    80103dfa <exit+0x8a>
      p->parent = initproc;
80103e16:	8b 0d 54 3c 11 80    	mov    0x80113c54,%ecx
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
80103e1c:	ba 54 1d 11 80       	mov    $0x80111d54,%edx
80103e21:	eb 10                	jmp    80103e33 <exit+0xc3>
80103e23:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
80103e28:	83 c2 7c             	add    $0x7c,%edx
80103e2b:	81 fa 54 3c 11 80    	cmp    $0x80113c54,%edx
80103e31:	74 3b                	je     80103e6e <exit+0xfe>
    if(p->parent == curproc){
80103e33:	39 5a 14             	cmp    %ebx,0x14(%edx)
80103e36:	75 f0                	jne    80103e28 <exit+0xb8>
      if(p->state == ZOMBIE)
80103e38:	83 7a 0c 05          	cmpl   $0x5,0xc(%edx)
      p->parent = initproc;
80103e3c:	89 4a 14             	mov    %ecx,0x14(%edx)
      if(p->state == ZOMBIE)
80103e3f:	75 e7                	jne    80103e28 <exit+0xb8>
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
80103e41:	b8 54 1d 11 80       	mov    $0x80111d54,%eax
80103e46:	eb 12                	jmp    80103e5a <exit+0xea>
80103e48:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80103e4f:	00 
80103e50:	83 c0 7c             	add    $0x7c,%eax
80103e53:	3d 54 3c 11 80       	cmp    $0x80113c54,%eax
80103e58:	74 ce                	je     80103e28 <exit+0xb8>
    if(p->state == SLEEPING && p->chan == chan)
80103e5a:	83 78 0c 02          	cmpl   $0x2,0xc(%eax)
80103e5e:	75 f0                	jne    80103e50 <exit+0xe0>
80103e60:	3b 48 20             	cmp    0x20(%eax),%ecx
80103e63:	75 eb                	jne    80103e50 <exit+0xe0>
      p->state = RUNNABLE;
80103e65:	c7 40 0c 03 00 00 00 	movl   $0x3,0xc(%eax)
80103e6c:	eb e2                	jmp    80103e50 <exit+0xe0>
  curproc->state = ZOMBIE;
80103e6e:	c7 43 0c 05 00 00 00 	movl   $0x5,0xc(%ebx)
  sched();
80103e75:	e8 36 fe ff ff       	call   80103cb0 <sched>
  panic("zombie exit");
80103e7a:	83 ec 0c             	sub    $0xc,%esp
80103e7d:	68 3e 74 10 80       	push   $0x8010743e
80103e82:	e8 f9 c4 ff ff       	call   80100380 <panic>
    panic("init exiting");
80103e87:	83 ec 0c             	sub    $0xc,%esp
80103e8a:	68 31 74 10 80       	push   $0x80107431
80103e8f:	e8 ec c4 ff ff       	call   80100380 <panic>
80103e94:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80103e9b:	00 
80103e9c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi

80103ea0 <wait>:
{
80103ea0:	55                   	push   %ebp
80103ea1:	89 e5                	mov    %esp,%ebp
80103ea3:	56                   	push   %esi
80103ea4:	53                   	push   %ebx
  pushcli();
80103ea5:	e8 a6 05 00 00       	call   80104450 <pushcli>
  c = mycpu();
80103eaa:	e8 31 fa ff ff       	call   801038e0 <mycpu>
  p = c->proc;
80103eaf:	8b b0 ac 00 00 00    	mov    0xac(%eax),%esi
  popcli();
80103eb5:	e8 e6 05 00 00       	call   801044a0 <popcli>
  acquire(&ptable.lock);
80103eba:	83 ec 0c             	sub    $0xc,%esp
80103ebd:	68 20 1d 11 80       	push   $0x80111d20
80103ec2:	e8 d9 06 00 00       	call   801045a0 <acquire>
80103ec7:	83 c4 10             	add    $0x10,%esp
    havekids = 0;
80103eca:	31 c0                	xor    %eax,%eax
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
80103ecc:	bb 54 1d 11 80       	mov    $0x80111d54,%ebx
80103ed1:	eb 10                	jmp    80103ee3 <wait+0x43>
80103ed3:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
80103ed8:	83 c3 7c             	add    $0x7c,%ebx
80103edb:	81 fb 54 3c 11 80    	cmp    $0x80113c54,%ebx
80103ee1:	74 1b                	je     80103efe <wait+0x5e>
      if(p->parent != curproc)
80103ee3:	39 73 14             	cmp    %esi,0x14(%ebx)
80103ee6:	75 f0                	jne    80103ed8 <wait+0x38>
      if(p->state == ZOMBIE){
80103ee8:	83 7b 0c 05          	cmpl   $0x5,0xc(%ebx)
80103eec:	74 62                	je     80103f50 <wait+0xb0>
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
80103eee:	83 c3 7c             	add    $0x7c,%ebx
      havekids = 1;
80103ef1:	b8 01 00 00 00       	mov    $0x1,%eax
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
80103ef6:	81 fb 54 3c 11 80    	cmp    $0x80113c54,%ebx
80103efc:	75 e5                	jne    80103ee3 <wait+0x43>
    if(!havekids || curproc->killed){
80103efe:	85 c0                	test   %eax,%eax
80103f00:	0f 84 a0 00 00 00    	je     80103fa6 <wait+0x106>
80103f06:	8b 46 24             	mov    0x24(%esi),%eax
80103f09:	85 c0                	test   %eax,%eax
80103f0b:	0f 85 95 00 00 00    	jne    80103fa6 <wait+0x106>
  pushcli();
80103f11:	e8 3a 05 00 00       	call   80104450 <pushcli>
  c = mycpu();
80103f16:	e8 c5 f9 ff ff       	call   801038e0 <mycpu>
  p = c->proc;
80103f1b:	8b 98 ac 00 00 00    	mov    0xac(%eax),%ebx
  popcli();
80103f21:	e8 7a 05 00 00       	call   801044a0 <popcli>
  if(p == 0)
80103f26:	85 db                	test   %ebx,%ebx
80103f28:	0f 84 8f 00 00 00    	je     80103fbd <wait+0x11d>
  p->chan = chan;
80103f2e:	89 73 20             	mov    %esi,0x20(%ebx)
  p->state = SLEEPING;
80103f31:	c7 43 0c 02 00 00 00 	movl   $0x2,0xc(%ebx)
  sched();
80103f38:	e8 73 fd ff ff       	call   80103cb0 <sched>
  p->chan = 0;
80103f3d:	c7 43 20 00 00 00 00 	movl   $0x0,0x20(%ebx)
}
80103f44:	eb 84                	jmp    80103eca <wait+0x2a>
80103f46:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80103f4d:	00 
80103f4e:	66 90                	xchg   %ax,%ax
        kfree(p->kstack);
80103f50:	83 ec 0c             	sub    $0xc,%esp
        pid = p->pid;
80103f53:	8b 73 10             	mov    0x10(%ebx),%esi
        kfree(p->kstack);
80103f56:	ff 73 08             	push   0x8(%ebx)
80103f59:	e8 42 e5 ff ff       	call   801024a0 <kfree>
        p->kstack = 0;
80103f5e:	c7 43 08 00 00 00 00 	movl   $0x0,0x8(%ebx)
        freevm(p->pgdir);
80103f65:	5a                   	pop    %edx
80103f66:	ff 73 04             	push   0x4(%ebx)
80103f69:	e8 32 2e 00 00       	call   80106da0 <freevm>
        p->pid = 0;
80103f6e:	c7 43 10 00 00 00 00 	movl   $0x0,0x10(%ebx)
        p->parent = 0;
80103f75:	c7 43 14 00 00 00 00 	movl   $0x0,0x14(%ebx)
        p->name[0] = 0;
80103f7c:	c6 43 6c 00          	movb   $0x0,0x6c(%ebx)
        p->killed = 0;
80103f80:	c7 43 24 00 00 00 00 	movl   $0x0,0x24(%ebx)
        p->state = UNUSED;
80103f87:	c7 43 0c 00 00 00 00 	movl   $0x0,0xc(%ebx)
        release(&ptable.lock);
80103f8e:	c7 04 24 20 1d 11 80 	movl   $0x80111d20,(%esp)
80103f95:	e8 a6 05 00 00       	call   80104540 <release>
        return pid;
80103f9a:	83 c4 10             	add    $0x10,%esp
}
80103f9d:	8d 65 f8             	lea    -0x8(%ebp),%esp
80103fa0:	89 f0                	mov    %esi,%eax
80103fa2:	5b                   	pop    %ebx
80103fa3:	5e                   	pop    %esi
80103fa4:	5d                   	pop    %ebp
80103fa5:	c3                   	ret
      release(&ptable.lock);
80103fa6:	83 ec 0c             	sub    $0xc,%esp
      return -1;
80103fa9:	be ff ff ff ff       	mov    $0xffffffff,%esi
      release(&ptable.lock);
80103fae:	68 20 1d 11 80       	push   $0x80111d20
80103fb3:	e8 88 05 00 00       	call   80104540 <release>
      return -1;
80103fb8:	83 c4 10             	add    $0x10,%esp
80103fbb:	eb e0                	jmp    80103f9d <wait+0xfd>
    panic("sleep");
80103fbd:	83 ec 0c             	sub    $0xc,%esp
80103fc0:	68 4a 74 10 80       	push   $0x8010744a
80103fc5:	e8 b6 c3 ff ff       	call   80100380 <panic>
80103fca:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi

80103fd0 <yield>:
{
80103fd0:	55                   	push   %ebp
80103fd1:	89 e5                	mov    %esp,%ebp
80103fd3:	53                   	push   %ebx
80103fd4:	83 ec 10             	sub    $0x10,%esp
  acquire(&ptable.lock);  //DOC: yieldlock
80103fd7:	68 20 1d 11 80       	push   $0x80111d20
80103fdc:	e8 bf 05 00 00       	call   801045a0 <acquire>
  pushcli();
80103fe1:	e8 6a 04 00 00       	call   80104450 <pushcli>
  c = mycpu();
80103fe6:	e8 f5 f8 ff ff       	call   801038e0 <mycpu>
  p = c->proc;
80103feb:	8b 98 ac 00 00 00    	mov    0xac(%eax),%ebx
  popcli();
80103ff1:	e8 aa 04 00 00       	call   801044a0 <popcli>
  myproc()->state = RUNNABLE;
80103ff6:	c7 43 0c 03 00 00 00 	movl   $0x3,0xc(%ebx)
  sched();
80103ffd:	e8 ae fc ff ff       	call   80103cb0 <sched>
  release(&ptable.lock);
80104002:	c7 04 24 20 1d 11 80 	movl   $0x80111d20,(%esp)
80104009:	e8 32 05 00 00       	call   80104540 <release>
}
8010400e:	8b 5d fc             	mov    -0x4(%ebp),%ebx
80104011:	83 c4 10             	add    $0x10,%esp
80104014:	c9                   	leave
80104015:	c3                   	ret
80104016:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010401d:	00 
8010401e:	66 90                	xchg   %ax,%ax

80104020 <sleep>:
{
80104020:	55                   	push   %ebp
80104021:	89 e5                	mov    %esp,%ebp
80104023:	57                   	push   %edi
80104024:	56                   	push   %esi
80104025:	53                   	push   %ebx
80104026:	83 ec 0c             	sub    $0xc,%esp
80104029:	8b 7d 08             	mov    0x8(%ebp),%edi
8010402c:	8b 75 0c             	mov    0xc(%ebp),%esi
  pushcli();
8010402f:	e8 1c 04 00 00       	call   80104450 <pushcli>
  c = mycpu();
80104034:	e8 a7 f8 ff ff       	call   801038e0 <mycpu>
  p = c->proc;
80104039:	8b 98 ac 00 00 00    	mov    0xac(%eax),%ebx
  popcli();
8010403f:	e8 5c 04 00 00       	call   801044a0 <popcli>
  if(p == 0)
80104044:	85 db                	test   %ebx,%ebx
80104046:	0f 84 87 00 00 00    	je     801040d3 <sleep+0xb3>
  if(lk == 0)
8010404c:	85 f6                	test   %esi,%esi
8010404e:	74 76                	je     801040c6 <sleep+0xa6>
  if(lk != &ptable.lock){  //DOC: sleeplock0
80104050:	81 fe 20 1d 11 80    	cmp    $0x80111d20,%esi
80104056:	74 50                	je     801040a8 <sleep+0x88>
    acquire(&ptable.lock);  //DOC: sleeplock1
80104058:	83 ec 0c             	sub    $0xc,%esp
8010405b:	68 20 1d 11 80       	push   $0x80111d20
80104060:	e8 3b 05 00 00       	call   801045a0 <acquire>
    release(lk);
80104065:	89 34 24             	mov    %esi,(%esp)
80104068:	e8 d3 04 00 00       	call   80104540 <release>
  p->chan = chan;
8010406d:	89 7b 20             	mov    %edi,0x20(%ebx)
  p->state = SLEEPING;
80104070:	c7 43 0c 02 00 00 00 	movl   $0x2,0xc(%ebx)
  sched();
80104077:	e8 34 fc ff ff       	call   80103cb0 <sched>
  p->chan = 0;
8010407c:	c7 43 20 00 00 00 00 	movl   $0x0,0x20(%ebx)
    release(&ptable.lock);
80104083:	c7 04 24 20 1d 11 80 	movl   $0x80111d20,(%esp)
8010408a:	e8 b1 04 00 00       	call   80104540 <release>
    acquire(lk);
8010408f:	83 c4 10             	add    $0x10,%esp
80104092:	89 75 08             	mov    %esi,0x8(%ebp)
}
80104095:	8d 65 f4             	lea    -0xc(%ebp),%esp
80104098:	5b                   	pop    %ebx
80104099:	5e                   	pop    %esi
8010409a:	5f                   	pop    %edi
8010409b:	5d                   	pop    %ebp
    acquire(lk);
8010409c:	e9 ff 04 00 00       	jmp    801045a0 <acquire>
801040a1:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
  p->chan = chan;
801040a8:	89 7b 20             	mov    %edi,0x20(%ebx)
  p->state = SLEEPING;
801040ab:	c7 43 0c 02 00 00 00 	movl   $0x2,0xc(%ebx)
  sched();
801040b2:	e8 f9 fb ff ff       	call   80103cb0 <sched>
  p->chan = 0;
801040b7:	c7 43 20 00 00 00 00 	movl   $0x0,0x20(%ebx)
}
801040be:	8d 65 f4             	lea    -0xc(%ebp),%esp
801040c1:	5b                   	pop    %ebx
801040c2:	5e                   	pop    %esi
801040c3:	5f                   	pop    %edi
801040c4:	5d                   	pop    %ebp
801040c5:	c3                   	ret
    panic("sleep without lk");
801040c6:	83 ec 0c             	sub    $0xc,%esp
801040c9:	68 50 74 10 80       	push   $0x80107450
801040ce:	e8 ad c2 ff ff       	call   80100380 <panic>
    panic("sleep");
801040d3:	83 ec 0c             	sub    $0xc,%esp
801040d6:	68 4a 74 10 80       	push   $0x8010744a
801040db:	e8 a0 c2 ff ff       	call   80100380 <panic>

801040e0 <wakeup>:
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
801040e0:	55                   	push   %ebp
801040e1:	89 e5                	mov    %esp,%ebp
801040e3:	53                   	push   %ebx
801040e4:	83 ec 10             	sub    $0x10,%esp
801040e7:	8b 5d 08             	mov    0x8(%ebp),%ebx
  acquire(&ptable.lock);
801040ea:	68 20 1d 11 80       	push   $0x80111d20
801040ef:	e8 ac 04 00 00       	call   801045a0 <acquire>
801040f4:	83 c4 10             	add    $0x10,%esp
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
801040f7:	b8 54 1d 11 80       	mov    $0x80111d54,%eax
801040fc:	eb 0c                	jmp    8010410a <wakeup+0x2a>
801040fe:	66 90                	xchg   %ax,%ax
80104100:	83 c0 7c             	add    $0x7c,%eax
80104103:	3d 54 3c 11 80       	cmp    $0x80113c54,%eax
80104108:	74 1c                	je     80104126 <wakeup+0x46>
    if(p->state == SLEEPING && p->chan == chan)
8010410a:	83 78 0c 02          	cmpl   $0x2,0xc(%eax)
8010410e:	75 f0                	jne    80104100 <wakeup+0x20>
80104110:	3b 58 20             	cmp    0x20(%eax),%ebx
80104113:	75 eb                	jne    80104100 <wakeup+0x20>
      p->state = RUNNABLE;
80104115:	c7 40 0c 03 00 00 00 	movl   $0x3,0xc(%eax)
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
8010411c:	83 c0 7c             	add    $0x7c,%eax
8010411f:	3d 54 3c 11 80       	cmp    $0x80113c54,%eax
80104124:	75 e4                	jne    8010410a <wakeup+0x2a>
  wakeup1(chan);
  release(&ptable.lock);
80104126:	c7 45 08 20 1d 11 80 	movl   $0x80111d20,0x8(%ebp)
}
8010412d:	8b 5d fc             	mov    -0x4(%ebp),%ebx
80104130:	c9                   	leave
  release(&ptable.lock);
80104131:	e9 0a 04 00 00       	jmp    80104540 <release>
80104136:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010413d:	00 
8010413e:	66 90                	xchg   %ax,%ax

80104140 <kill>:
// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
80104140:	55                   	push   %ebp
80104141:	89 e5                	mov    %esp,%ebp
80104143:	53                   	push   %ebx
80104144:	83 ec 10             	sub    $0x10,%esp
80104147:	8b 5d 08             	mov    0x8(%ebp),%ebx
  struct proc *p;

  acquire(&ptable.lock);
8010414a:	68 20 1d 11 80       	push   $0x80111d20
8010414f:	e8 4c 04 00 00       	call   801045a0 <acquire>
80104154:	83 c4 10             	add    $0x10,%esp
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
80104157:	b8 54 1d 11 80       	mov    $0x80111d54,%eax
8010415c:	eb 0c                	jmp    8010416a <kill+0x2a>
8010415e:	66 90                	xchg   %ax,%ax
80104160:	83 c0 7c             	add    $0x7c,%eax
80104163:	3d 54 3c 11 80       	cmp    $0x80113c54,%eax
80104168:	74 36                	je     801041a0 <kill+0x60>
    if(p->pid == pid){
8010416a:	39 58 10             	cmp    %ebx,0x10(%eax)
8010416d:	75 f1                	jne    80104160 <kill+0x20>
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
8010416f:	83 78 0c 02          	cmpl   $0x2,0xc(%eax)
      p->killed = 1;
80104173:	c7 40 24 01 00 00 00 	movl   $0x1,0x24(%eax)
      if(p->state == SLEEPING)
8010417a:	75 07                	jne    80104183 <kill+0x43>
        p->state = RUNNABLE;
8010417c:	c7 40 0c 03 00 00 00 	movl   $0x3,0xc(%eax)
      release(&ptable.lock);
80104183:	83 ec 0c             	sub    $0xc,%esp
80104186:	68 20 1d 11 80       	push   $0x80111d20
8010418b:	e8 b0 03 00 00       	call   80104540 <release>
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}
80104190:	8b 5d fc             	mov    -0x4(%ebp),%ebx
      return 0;
80104193:	83 c4 10             	add    $0x10,%esp
80104196:	31 c0                	xor    %eax,%eax
}
80104198:	c9                   	leave
80104199:	c3                   	ret
8010419a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
  release(&ptable.lock);
801041a0:	83 ec 0c             	sub    $0xc,%esp
801041a3:	68 20 1d 11 80       	push   $0x80111d20
801041a8:	e8 93 03 00 00       	call   80104540 <release>
}
801041ad:	8b 5d fc             	mov    -0x4(%ebp),%ebx
  return -1;
801041b0:	83 c4 10             	add    $0x10,%esp
801041b3:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
}
801041b8:	c9                   	leave
801041b9:	c3                   	ret
801041ba:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi

801041c0 <procdump>:
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
801041c0:	55                   	push   %ebp
801041c1:	89 e5                	mov    %esp,%ebp
801041c3:	57                   	push   %edi
801041c4:	56                   	push   %esi
801041c5:	8d 75 e8             	lea    -0x18(%ebp),%esi
801041c8:	53                   	push   %ebx
801041c9:	bb c0 1d 11 80       	mov    $0x80111dc0,%ebx
801041ce:	83 ec 3c             	sub    $0x3c,%esp
801041d1:	eb 24                	jmp    801041f7 <procdump+0x37>
801041d3:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
801041d8:	83 ec 0c             	sub    $0xc,%esp
801041db:	68 0f 76 10 80       	push   $0x8010760f
801041e0:	e8 cb c4 ff ff       	call   801006b0 <cprintf>
801041e5:	83 c4 10             	add    $0x10,%esp
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
801041e8:	83 c3 7c             	add    $0x7c,%ebx
801041eb:	81 fb c0 3c 11 80    	cmp    $0x80113cc0,%ebx
801041f1:	0f 84 81 00 00 00    	je     80104278 <procdump+0xb8>
    if(p->state == UNUSED)
801041f7:	8b 43 a0             	mov    -0x60(%ebx),%eax
801041fa:	85 c0                	test   %eax,%eax
801041fc:	74 ea                	je     801041e8 <procdump+0x28>
      state = "???";
801041fe:	ba 61 74 10 80       	mov    $0x80107461,%edx
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
80104203:	83 f8 05             	cmp    $0x5,%eax
80104206:	77 11                	ja     80104219 <procdump+0x59>
80104208:	8b 14 85 40 7a 10 80 	mov    -0x7fef85c0(,%eax,4),%edx
      state = "???";
8010420f:	b8 61 74 10 80       	mov    $0x80107461,%eax
80104214:	85 d2                	test   %edx,%edx
80104216:	0f 44 d0             	cmove  %eax,%edx
    cprintf("%d %s %s", p->pid, state, p->name);
80104219:	53                   	push   %ebx
8010421a:	52                   	push   %edx
8010421b:	ff 73 a4             	push   -0x5c(%ebx)
8010421e:	68 65 74 10 80       	push   $0x80107465
80104223:	e8 88 c4 ff ff       	call   801006b0 <cprintf>
    if(p->state == SLEEPING){
80104228:	83 c4 10             	add    $0x10,%esp
8010422b:	83 7b a0 02          	cmpl   $0x2,-0x60(%ebx)
8010422f:	75 a7                	jne    801041d8 <procdump+0x18>
      getcallerpcs((uint*)p->context->ebp+2, pc);
80104231:	83 ec 08             	sub    $0x8,%esp
80104234:	8d 45 c0             	lea    -0x40(%ebp),%eax
80104237:	8d 7d c0             	lea    -0x40(%ebp),%edi
8010423a:	50                   	push   %eax
8010423b:	8b 43 b0             	mov    -0x50(%ebx),%eax
8010423e:	8b 40 0c             	mov    0xc(%eax),%eax
80104241:	83 c0 08             	add    $0x8,%eax
80104244:	50                   	push   %eax
80104245:	e8 86 01 00 00       	call   801043d0 <getcallerpcs>
      for(i=0; i<10 && pc[i] != 0; i++)
8010424a:	83 c4 10             	add    $0x10,%esp
8010424d:	8d 76 00             	lea    0x0(%esi),%esi
80104250:	8b 17                	mov    (%edi),%edx
80104252:	85 d2                	test   %edx,%edx
80104254:	74 82                	je     801041d8 <procdump+0x18>
        cprintf(" %p", pc[i]);
80104256:	83 ec 08             	sub    $0x8,%esp
      for(i=0; i<10 && pc[i] != 0; i++)
80104259:	83 c7 04             	add    $0x4,%edi
        cprintf(" %p", pc[i]);
8010425c:	52                   	push   %edx
8010425d:	68 a1 71 10 80       	push   $0x801071a1
80104262:	e8 49 c4 ff ff       	call   801006b0 <cprintf>
      for(i=0; i<10 && pc[i] != 0; i++)
80104267:	83 c4 10             	add    $0x10,%esp
8010426a:	39 f7                	cmp    %esi,%edi
8010426c:	75 e2                	jne    80104250 <procdump+0x90>
8010426e:	e9 65 ff ff ff       	jmp    801041d8 <procdump+0x18>
80104273:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
  }
}
80104278:	8d 65 f4             	lea    -0xc(%ebp),%esp
8010427b:	5b                   	pop    %ebx
8010427c:	5e                   	pop    %esi
8010427d:	5f                   	pop    %edi
8010427e:	5d                   	pop    %ebp
8010427f:	c3                   	ret

80104280 <initsleeplock>:
#include "spinlock.h"
#include "sleeplock.h"

void
initsleeplock(struct sleeplock *lk, char *name)
{
80104280:	55                   	push   %ebp
80104281:	89 e5                	mov    %esp,%ebp
80104283:	53                   	push   %ebx
80104284:	83 ec 0c             	sub    $0xc,%esp
80104287:	8b 5d 08             	mov    0x8(%ebp),%ebx
  initlock(&lk->lk, "sleep lock");
8010428a:	68 98 74 10 80       	push   $0x80107498
8010428f:	8d 43 04             	lea    0x4(%ebx),%eax
80104292:	50                   	push   %eax
80104293:	e8 18 01 00 00       	call   801043b0 <initlock>
  lk->name = name;
80104298:	8b 45 0c             	mov    0xc(%ebp),%eax
  lk->locked = 0;
8010429b:	c7 03 00 00 00 00    	movl   $0x0,(%ebx)
  lk->pid = 0;
}
801042a1:	83 c4 10             	add    $0x10,%esp
  lk->pid = 0;
801042a4:	c7 43 3c 00 00 00 00 	movl   $0x0,0x3c(%ebx)
  lk->name = name;
801042ab:	89 43 38             	mov    %eax,0x38(%ebx)
}
801042ae:	8b 5d fc             	mov    -0x4(%ebp),%ebx
801042b1:	c9                   	leave
801042b2:	c3                   	ret
801042b3:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801042ba:	00 
801042bb:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi

801042c0 <acquiresleep>:

void
acquiresleep(struct sleeplock *lk)
{
801042c0:	55                   	push   %ebp
801042c1:	89 e5                	mov    %esp,%ebp
801042c3:	56                   	push   %esi
801042c4:	53                   	push   %ebx
801042c5:	8b 5d 08             	mov    0x8(%ebp),%ebx
  acquire(&lk->lk);
801042c8:	8d 73 04             	lea    0x4(%ebx),%esi
801042cb:	83 ec 0c             	sub    $0xc,%esp
801042ce:	56                   	push   %esi
801042cf:	e8 cc 02 00 00       	call   801045a0 <acquire>
  while (lk->locked) {
801042d4:	8b 13                	mov    (%ebx),%edx
801042d6:	83 c4 10             	add    $0x10,%esp
801042d9:	85 d2                	test   %edx,%edx
801042db:	74 16                	je     801042f3 <acquiresleep+0x33>
801042dd:	8d 76 00             	lea    0x0(%esi),%esi
    sleep(lk, &lk->lk);
801042e0:	83 ec 08             	sub    $0x8,%esp
801042e3:	56                   	push   %esi
801042e4:	53                   	push   %ebx
801042e5:	e8 36 fd ff ff       	call   80104020 <sleep>
  while (lk->locked) {
801042ea:	8b 03                	mov    (%ebx),%eax
801042ec:	83 c4 10             	add    $0x10,%esp
801042ef:	85 c0                	test   %eax,%eax
801042f1:	75 ed                	jne    801042e0 <acquiresleep+0x20>
  }
  lk->locked = 1;
801042f3:	c7 03 01 00 00 00    	movl   $0x1,(%ebx)
  lk->pid = myproc()->pid;
801042f9:	e8 62 f6 ff ff       	call   80103960 <myproc>
801042fe:	8b 40 10             	mov    0x10(%eax),%eax
80104301:	89 43 3c             	mov    %eax,0x3c(%ebx)
  release(&lk->lk);
80104304:	89 75 08             	mov    %esi,0x8(%ebp)
}
80104307:	8d 65 f8             	lea    -0x8(%ebp),%esp
8010430a:	5b                   	pop    %ebx
8010430b:	5e                   	pop    %esi
8010430c:	5d                   	pop    %ebp
  release(&lk->lk);
8010430d:	e9 2e 02 00 00       	jmp    80104540 <release>
80104312:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80104319:	00 
8010431a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi

80104320 <releasesleep>:

void
releasesleep(struct sleeplock *lk)
{
80104320:	55                   	push   %ebp
80104321:	89 e5                	mov    %esp,%ebp
80104323:	56                   	push   %esi
80104324:	53                   	push   %ebx
80104325:	8b 5d 08             	mov    0x8(%ebp),%ebx
  acquire(&lk->lk);
80104328:	8d 73 04             	lea    0x4(%ebx),%esi
8010432b:	83 ec 0c             	sub    $0xc,%esp
8010432e:	56                   	push   %esi
8010432f:	e8 6c 02 00 00       	call   801045a0 <acquire>
  lk->locked = 0;
80104334:	c7 03 00 00 00 00    	movl   $0x0,(%ebx)
  lk->pid = 0;
8010433a:	c7 43 3c 00 00 00 00 	movl   $0x0,0x3c(%ebx)
  wakeup(lk);
80104341:	89 1c 24             	mov    %ebx,(%esp)
80104344:	e8 97 fd ff ff       	call   801040e0 <wakeup>
  release(&lk->lk);
80104349:	83 c4 10             	add    $0x10,%esp
8010434c:	89 75 08             	mov    %esi,0x8(%ebp)
}
8010434f:	8d 65 f8             	lea    -0x8(%ebp),%esp
80104352:	5b                   	pop    %ebx
80104353:	5e                   	pop    %esi
80104354:	5d                   	pop    %ebp
  release(&lk->lk);
80104355:	e9 e6 01 00 00       	jmp    80104540 <release>
8010435a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi

80104360 <holdingsleep>:

int
holdingsleep(struct sleeplock *lk)
{
80104360:	55                   	push   %ebp
80104361:	89 e5                	mov    %esp,%ebp
80104363:	57                   	push   %edi
80104364:	31 ff                	xor    %edi,%edi
80104366:	56                   	push   %esi
80104367:	53                   	push   %ebx
80104368:	83 ec 18             	sub    $0x18,%esp
8010436b:	8b 5d 08             	mov    0x8(%ebp),%ebx
  int r;
  
  acquire(&lk->lk);
8010436e:	8d 73 04             	lea    0x4(%ebx),%esi
80104371:	56                   	push   %esi
80104372:	e8 29 02 00 00       	call   801045a0 <acquire>
  r = lk->locked && (lk->pid == myproc()->pid);
80104377:	8b 03                	mov    (%ebx),%eax
80104379:	83 c4 10             	add    $0x10,%esp
8010437c:	85 c0                	test   %eax,%eax
8010437e:	75 18                	jne    80104398 <holdingsleep+0x38>
  release(&lk->lk);
80104380:	83 ec 0c             	sub    $0xc,%esp
80104383:	56                   	push   %esi
80104384:	e8 b7 01 00 00       	call   80104540 <release>
  return r;
}
80104389:	8d 65 f4             	lea    -0xc(%ebp),%esp
8010438c:	89 f8                	mov    %edi,%eax
8010438e:	5b                   	pop    %ebx
8010438f:	5e                   	pop    %esi
80104390:	5f                   	pop    %edi
80104391:	5d                   	pop    %ebp
80104392:	c3                   	ret
80104393:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
  r = lk->locked && (lk->pid == myproc()->pid);
80104398:	8b 5b 3c             	mov    0x3c(%ebx),%ebx
8010439b:	e8 c0 f5 ff ff       	call   80103960 <myproc>
801043a0:	39 58 10             	cmp    %ebx,0x10(%eax)
801043a3:	0f 94 c0             	sete   %al
801043a6:	0f b6 c0             	movzbl %al,%eax
801043a9:	89 c7                	mov    %eax,%edi
801043ab:	eb d3                	jmp    80104380 <holdingsleep+0x20>
801043ad:	66 90                	xchg   %ax,%ax
801043af:	90                   	nop

801043b0 <initlock>:
#include "proc.h"
#include "spinlock.h"

void
initlock(struct spinlock *lk, char *name)
{
801043b0:	55                   	push   %ebp
801043b1:	89 e5                	mov    %esp,%ebp
801043b3:	8b 45 08             	mov    0x8(%ebp),%eax
  lk->name = name;
801043b6:	8b 55 0c             	mov    0xc(%ebp),%edx
  lk->locked = 0;
801043b9:	c7 00 00 00 00 00    	movl   $0x0,(%eax)
  lk->name = name;
801043bf:	89 50 04             	mov    %edx,0x4(%eax)
  lk->cpu = 0;
801043c2:	c7 40 08 00 00 00 00 	movl   $0x0,0x8(%eax)
}
801043c9:	5d                   	pop    %ebp
801043ca:	c3                   	ret
801043cb:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi

801043d0 <getcallerpcs>:
}

// Record the current call stack in pcs[] by following the %ebp chain.
void
getcallerpcs(void *v, uint pcs[])
{
801043d0:	55                   	push   %ebp
801043d1:	89 e5                	mov    %esp,%ebp
801043d3:	53                   	push   %ebx
801043d4:	8b 45 08             	mov    0x8(%ebp),%eax
801043d7:	8b 4d 0c             	mov    0xc(%ebp),%ecx
  uint *ebp;
  int i;

  ebp = (uint*)v - 2;
801043da:	8d 50 f8             	lea    -0x8(%eax),%edx
  for(i = 0; i < 10; i++){
    if(ebp == 0 || ebp < (uint*)KERNBASE || ebp == (uint*)0xffffffff)
801043dd:	05 f8 ff ff 7f       	add    $0x7ffffff8,%eax
801043e2:	3d fe ff ff 7f       	cmp    $0x7ffffffe,%eax
  for(i = 0; i < 10; i++){
801043e7:	b8 00 00 00 00       	mov    $0x0,%eax
    if(ebp == 0 || ebp < (uint*)KERNBASE || ebp == (uint*)0xffffffff)
801043ec:	76 10                	jbe    801043fe <getcallerpcs+0x2e>
801043ee:	eb 28                	jmp    80104418 <getcallerpcs+0x48>
801043f0:	8d 9a 00 00 00 80    	lea    -0x80000000(%edx),%ebx
801043f6:	81 fb fe ff ff 7f    	cmp    $0x7ffffffe,%ebx
801043fc:	77 1a                	ja     80104418 <getcallerpcs+0x48>
      break;
    pcs[i] = ebp[1];     // saved %eip
801043fe:	8b 5a 04             	mov    0x4(%edx),%ebx
80104401:	89 1c 81             	mov    %ebx,(%ecx,%eax,4)
  for(i = 0; i < 10; i++){
80104404:	83 c0 01             	add    $0x1,%eax
    ebp = (uint*)ebp[0]; // saved %ebp
80104407:	8b 12                	mov    (%edx),%edx
  for(i = 0; i < 10; i++){
80104409:	83 f8 0a             	cmp    $0xa,%eax
8010440c:	75 e2                	jne    801043f0 <getcallerpcs+0x20>
  }
  for(; i < 10; i++)
    pcs[i] = 0;
}
8010440e:	8b 5d fc             	mov    -0x4(%ebp),%ebx
80104411:	c9                   	leave
80104412:	c3                   	ret
80104413:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
80104418:	8d 04 81             	lea    (%ecx,%eax,4),%eax
8010441b:	83 c1 28             	add    $0x28,%ecx
8010441e:	89 ca                	mov    %ecx,%edx
80104420:	29 c2                	sub    %eax,%edx
80104422:	83 e2 04             	and    $0x4,%edx
80104425:	74 11                	je     80104438 <getcallerpcs+0x68>
    pcs[i] = 0;
80104427:	c7 00 00 00 00 00    	movl   $0x0,(%eax)
  for(; i < 10; i++)
8010442d:	83 c0 04             	add    $0x4,%eax
80104430:	39 c1                	cmp    %eax,%ecx
80104432:	74 da                	je     8010440e <getcallerpcs+0x3e>
80104434:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    pcs[i] = 0;
80104438:	c7 00 00 00 00 00    	movl   $0x0,(%eax)
  for(; i < 10; i++)
8010443e:	83 c0 08             	add    $0x8,%eax
    pcs[i] = 0;
80104441:	c7 40 fc 00 00 00 00 	movl   $0x0,-0x4(%eax)
  for(; i < 10; i++)
80104448:	39 c1                	cmp    %eax,%ecx
8010444a:	75 ec                	jne    80104438 <getcallerpcs+0x68>
8010444c:	eb c0                	jmp    8010440e <getcallerpcs+0x3e>
8010444e:	66 90                	xchg   %ax,%ax

80104450 <pushcli>:
// it takes two popcli to undo two pushcli.  Also, if interrupts
// are off, then pushcli, popcli leaves them off.

void
pushcli(void)
{
80104450:	55                   	push   %ebp
80104451:	89 e5                	mov    %esp,%ebp
80104453:	53                   	push   %ebx
80104454:	83 ec 04             	sub    $0x4,%esp
80104457:	9c                   	pushf
80104458:	5b                   	pop    %ebx
  asm volatile("cli");
80104459:	fa                   	cli
  int eflags;

  eflags = readeflags();
  cli();
  if(mycpu()->ncli == 0)
8010445a:	e8 81 f4 ff ff       	call   801038e0 <mycpu>
8010445f:	8b 80 a4 00 00 00    	mov    0xa4(%eax),%eax
80104465:	85 c0                	test   %eax,%eax
80104467:	74 17                	je     80104480 <pushcli+0x30>
    mycpu()->intena = eflags & FL_IF;
  mycpu()->ncli += 1;
80104469:	e8 72 f4 ff ff       	call   801038e0 <mycpu>
8010446e:	83 80 a4 00 00 00 01 	addl   $0x1,0xa4(%eax)
}
80104475:	8b 5d fc             	mov    -0x4(%ebp),%ebx
80104478:	c9                   	leave
80104479:	c3                   	ret
8010447a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    mycpu()->intena = eflags & FL_IF;
80104480:	e8 5b f4 ff ff       	call   801038e0 <mycpu>
80104485:	81 e3 00 02 00 00    	and    $0x200,%ebx
8010448b:	89 98 a8 00 00 00    	mov    %ebx,0xa8(%eax)
80104491:	eb d6                	jmp    80104469 <pushcli+0x19>
80104493:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010449a:	00 
8010449b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi

801044a0 <popcli>:

void
popcli(void)
{
801044a0:	55                   	push   %ebp
801044a1:	89 e5                	mov    %esp,%ebp
801044a3:	83 ec 08             	sub    $0x8,%esp
  asm volatile("pushfl; popl %0" : "=r" (eflags));
801044a6:	9c                   	pushf
801044a7:	58                   	pop    %eax
  if(readeflags()&FL_IF)
801044a8:	f6 c4 02             	test   $0x2,%ah
801044ab:	75 35                	jne    801044e2 <popcli+0x42>
    panic("popcli - interruptible");
  if(--mycpu()->ncli < 0)
801044ad:	e8 2e f4 ff ff       	call   801038e0 <mycpu>
801044b2:	83 a8 a4 00 00 00 01 	subl   $0x1,0xa4(%eax)
801044b9:	78 34                	js     801044ef <popcli+0x4f>
    panic("popcli");
  if(mycpu()->ncli == 0 && mycpu()->intena)
801044bb:	e8 20 f4 ff ff       	call   801038e0 <mycpu>
801044c0:	8b 90 a4 00 00 00    	mov    0xa4(%eax),%edx
801044c6:	85 d2                	test   %edx,%edx
801044c8:	74 06                	je     801044d0 <popcli+0x30>
    sti();
}
801044ca:	c9                   	leave
801044cb:	c3                   	ret
801044cc:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
  if(mycpu()->ncli == 0 && mycpu()->intena)
801044d0:	e8 0b f4 ff ff       	call   801038e0 <mycpu>
801044d5:	8b 80 a8 00 00 00    	mov    0xa8(%eax),%eax
801044db:	85 c0                	test   %eax,%eax
801044dd:	74 eb                	je     801044ca <popcli+0x2a>
  asm volatile("sti");
801044df:	fb                   	sti
}
801044e0:	c9                   	leave
801044e1:	c3                   	ret
    panic("popcli - interruptible");
801044e2:	83 ec 0c             	sub    $0xc,%esp
801044e5:	68 a3 74 10 80       	push   $0x801074a3
801044ea:	e8 91 be ff ff       	call   80100380 <panic>
    panic("popcli");
801044ef:	83 ec 0c             	sub    $0xc,%esp
801044f2:	68 ba 74 10 80       	push   $0x801074ba
801044f7:	e8 84 be ff ff       	call   80100380 <panic>
801044fc:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi

80104500 <holding>:
{
80104500:	55                   	push   %ebp
80104501:	89 e5                	mov    %esp,%ebp
80104503:	56                   	push   %esi
80104504:	53                   	push   %ebx
80104505:	8b 75 08             	mov    0x8(%ebp),%esi
80104508:	31 db                	xor    %ebx,%ebx
  pushcli();
8010450a:	e8 41 ff ff ff       	call   80104450 <pushcli>
  r = lock->locked && lock->cpu == mycpu();
8010450f:	8b 06                	mov    (%esi),%eax
80104511:	85 c0                	test   %eax,%eax
80104513:	75 0b                	jne    80104520 <holding+0x20>
  popcli();
80104515:	e8 86 ff ff ff       	call   801044a0 <popcli>
}
8010451a:	89 d8                	mov    %ebx,%eax
8010451c:	5b                   	pop    %ebx
8010451d:	5e                   	pop    %esi
8010451e:	5d                   	pop    %ebp
8010451f:	c3                   	ret
  r = lock->locked && lock->cpu == mycpu();
80104520:	8b 5e 08             	mov    0x8(%esi),%ebx
80104523:	e8 b8 f3 ff ff       	call   801038e0 <mycpu>
80104528:	39 c3                	cmp    %eax,%ebx
8010452a:	0f 94 c3             	sete   %bl
  popcli();
8010452d:	e8 6e ff ff ff       	call   801044a0 <popcli>
  r = lock->locked && lock->cpu == mycpu();
80104532:	0f b6 db             	movzbl %bl,%ebx
}
80104535:	89 d8                	mov    %ebx,%eax
80104537:	5b                   	pop    %ebx
80104538:	5e                   	pop    %esi
80104539:	5d                   	pop    %ebp
8010453a:	c3                   	ret
8010453b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi

80104540 <release>:
{
80104540:	55                   	push   %ebp
80104541:	89 e5                	mov    %esp,%ebp
80104543:	56                   	push   %esi
80104544:	53                   	push   %ebx
80104545:	8b 5d 08             	mov    0x8(%ebp),%ebx
  pushcli();
80104548:	e8 03 ff ff ff       	call   80104450 <pushcli>
  r = lock->locked && lock->cpu == mycpu();
8010454d:	8b 03                	mov    (%ebx),%eax
8010454f:	85 c0                	test   %eax,%eax
80104551:	75 15                	jne    80104568 <release+0x28>
  popcli();
80104553:	e8 48 ff ff ff       	call   801044a0 <popcli>
    panic("release");
80104558:	83 ec 0c             	sub    $0xc,%esp
8010455b:	68 c1 74 10 80       	push   $0x801074c1
80104560:	e8 1b be ff ff       	call   80100380 <panic>
80104565:	8d 76 00             	lea    0x0(%esi),%esi
  r = lock->locked && lock->cpu == mycpu();
80104568:	8b 73 08             	mov    0x8(%ebx),%esi
8010456b:	e8 70 f3 ff ff       	call   801038e0 <mycpu>
80104570:	39 c6                	cmp    %eax,%esi
80104572:	75 df                	jne    80104553 <release+0x13>
  popcli();
80104574:	e8 27 ff ff ff       	call   801044a0 <popcli>
  lk->pcs[0] = 0;
80104579:	c7 43 0c 00 00 00 00 	movl   $0x0,0xc(%ebx)
  lk->cpu = 0;
80104580:	c7 43 08 00 00 00 00 	movl   $0x0,0x8(%ebx)
  __sync_synchronize();
80104587:	f0 83 0c 24 00       	lock orl $0x0,(%esp)
  asm volatile("movl $0, %0" : "+m" (lk->locked) : );
8010458c:	c7 03 00 00 00 00    	movl   $0x0,(%ebx)
}
80104592:	8d 65 f8             	lea    -0x8(%ebp),%esp
80104595:	5b                   	pop    %ebx
80104596:	5e                   	pop    %esi
80104597:	5d                   	pop    %ebp
  popcli();
80104598:	e9 03 ff ff ff       	jmp    801044a0 <popcli>
8010459d:	8d 76 00             	lea    0x0(%esi),%esi

801045a0 <acquire>:
{
801045a0:	55                   	push   %ebp
801045a1:	89 e5                	mov    %esp,%ebp
801045a3:	53                   	push   %ebx
801045a4:	83 ec 04             	sub    $0x4,%esp
  pushcli(); // disable interrupts to avoid deadlock.
801045a7:	e8 a4 fe ff ff       	call   80104450 <pushcli>
  if(holding(lk))
801045ac:	8b 5d 08             	mov    0x8(%ebp),%ebx
  pushcli();
801045af:	e8 9c fe ff ff       	call   80104450 <pushcli>
  r = lock->locked && lock->cpu == mycpu();
801045b4:	8b 03                	mov    (%ebx),%eax
801045b6:	85 c0                	test   %eax,%eax
801045b8:	0f 85 b2 00 00 00    	jne    80104670 <acquire+0xd0>
  popcli();
801045be:	e8 dd fe ff ff       	call   801044a0 <popcli>
  asm volatile("lock; xchgl %0, %1" :
801045c3:	b9 01 00 00 00       	mov    $0x1,%ecx
801045c8:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801045cf:	00 
  while(xchg(&lk->locked, 1) != 0)
801045d0:	8b 55 08             	mov    0x8(%ebp),%edx
801045d3:	89 c8                	mov    %ecx,%eax
801045d5:	f0 87 02             	lock xchg %eax,(%edx)
801045d8:	85 c0                	test   %eax,%eax
801045da:	75 f4                	jne    801045d0 <acquire+0x30>
  __sync_synchronize();
801045dc:	f0 83 0c 24 00       	lock orl $0x0,(%esp)
  lk->cpu = mycpu();
801045e1:	8b 5d 08             	mov    0x8(%ebp),%ebx
801045e4:	e8 f7 f2 ff ff       	call   801038e0 <mycpu>
  getcallerpcs(&lk, lk->pcs);
801045e9:	8b 4d 08             	mov    0x8(%ebp),%ecx
  for(i = 0; i < 10; i++){
801045ec:	31 d2                	xor    %edx,%edx
  lk->cpu = mycpu();
801045ee:	89 43 08             	mov    %eax,0x8(%ebx)
    if(ebp == 0 || ebp < (uint*)KERNBASE || ebp == (uint*)0xffffffff)
801045f1:	8d 85 00 00 00 80    	lea    -0x80000000(%ebp),%eax
801045f7:	3d fe ff ff 7f       	cmp    $0x7ffffffe,%eax
801045fc:	77 32                	ja     80104630 <acquire+0x90>
  ebp = (uint*)v - 2;
801045fe:	89 e8                	mov    %ebp,%eax
80104600:	eb 14                	jmp    80104616 <acquire+0x76>
80104602:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    if(ebp == 0 || ebp < (uint*)KERNBASE || ebp == (uint*)0xffffffff)
80104608:	8d 98 00 00 00 80    	lea    -0x80000000(%eax),%ebx
8010460e:	81 fb fe ff ff 7f    	cmp    $0x7ffffffe,%ebx
80104614:	77 1a                	ja     80104630 <acquire+0x90>
    pcs[i] = ebp[1];     // saved %eip
80104616:	8b 58 04             	mov    0x4(%eax),%ebx
80104619:	89 5c 91 0c          	mov    %ebx,0xc(%ecx,%edx,4)
  for(i = 0; i < 10; i++){
8010461d:	83 c2 01             	add    $0x1,%edx
    ebp = (uint*)ebp[0]; // saved %ebp
80104620:	8b 00                	mov    (%eax),%eax
  for(i = 0; i < 10; i++){
80104622:	83 fa 0a             	cmp    $0xa,%edx
80104625:	75 e1                	jne    80104608 <acquire+0x68>
}
80104627:	8b 5d fc             	mov    -0x4(%ebp),%ebx
8010462a:	c9                   	leave
8010462b:	c3                   	ret
8010462c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
80104630:	8d 44 91 0c          	lea    0xc(%ecx,%edx,4),%eax
80104634:	83 c1 34             	add    $0x34,%ecx
80104637:	89 ca                	mov    %ecx,%edx
80104639:	29 c2                	sub    %eax,%edx
8010463b:	83 e2 04             	and    $0x4,%edx
8010463e:	74 10                	je     80104650 <acquire+0xb0>
    pcs[i] = 0;
80104640:	c7 00 00 00 00 00    	movl   $0x0,(%eax)
  for(; i < 10; i++)
80104646:	83 c0 04             	add    $0x4,%eax
80104649:	39 c1                	cmp    %eax,%ecx
8010464b:	74 da                	je     80104627 <acquire+0x87>
8010464d:	8d 76 00             	lea    0x0(%esi),%esi
    pcs[i] = 0;
80104650:	c7 00 00 00 00 00    	movl   $0x0,(%eax)
  for(; i < 10; i++)
80104656:	83 c0 08             	add    $0x8,%eax
    pcs[i] = 0;
80104659:	c7 40 fc 00 00 00 00 	movl   $0x0,-0x4(%eax)
  for(; i < 10; i++)
80104660:	39 c1                	cmp    %eax,%ecx
80104662:	75 ec                	jne    80104650 <acquire+0xb0>
80104664:	eb c1                	jmp    80104627 <acquire+0x87>
80104666:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010466d:	00 
8010466e:	66 90                	xchg   %ax,%ax
  r = lock->locked && lock->cpu == mycpu();
80104670:	8b 5b 08             	mov    0x8(%ebx),%ebx
80104673:	e8 68 f2 ff ff       	call   801038e0 <mycpu>
80104678:	39 c3                	cmp    %eax,%ebx
8010467a:	0f 85 3e ff ff ff    	jne    801045be <acquire+0x1e>
  popcli();
80104680:	e8 1b fe ff ff       	call   801044a0 <popcli>
    panic("acquire");
80104685:	83 ec 0c             	sub    $0xc,%esp
80104688:	68 c9 74 10 80       	push   $0x801074c9
8010468d:	e8 ee bc ff ff       	call   80100380 <panic>
80104692:	66 90                	xchg   %ax,%ax
80104694:	66 90                	xchg   %ax,%ax
80104696:	66 90                	xchg   %ax,%ax
80104698:	66 90                	xchg   %ax,%ax
8010469a:	66 90                	xchg   %ax,%ax
8010469c:	66 90                	xchg   %ax,%ax
8010469e:	66 90                	xchg   %ax,%ax

801046a0 <memset>:
#include "types.h"
#include "x86.h"

void*
memset(void *dst, int c, uint n)
{
801046a0:	55                   	push   %ebp
801046a1:	89 e5                	mov    %esp,%ebp
801046a3:	57                   	push   %edi
801046a4:	8b 55 08             	mov    0x8(%ebp),%edx
801046a7:	8b 4d 10             	mov    0x10(%ebp),%ecx
  if ((int)dst%4 == 0 && n%4 == 0){
801046aa:	89 d0                	mov    %edx,%eax
801046ac:	09 c8                	or     %ecx,%eax
801046ae:	a8 03                	test   $0x3,%al
801046b0:	75 1e                	jne    801046d0 <memset+0x30>
    c &= 0xFF;
801046b2:	0f b6 45 0c          	movzbl 0xc(%ebp),%eax
    stosl(dst, (c<<24)|(c<<16)|(c<<8)|c, n/4);
801046b6:	c1 e9 02             	shr    $0x2,%ecx
  asm volatile("cld; rep stosl" :
801046b9:	89 d7                	mov    %edx,%edi
801046bb:	69 c0 01 01 01 01    	imul   $0x1010101,%eax,%eax
801046c1:	fc                   	cld
801046c2:	f3 ab                	rep stos %eax,%es:(%edi)
  } else
    stosb(dst, c, n);
  return dst;
}
801046c4:	8b 7d fc             	mov    -0x4(%ebp),%edi
801046c7:	89 d0                	mov    %edx,%eax
801046c9:	c9                   	leave
801046ca:	c3                   	ret
801046cb:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
  asm volatile("cld; rep stosb" :
801046d0:	8b 45 0c             	mov    0xc(%ebp),%eax
801046d3:	89 d7                	mov    %edx,%edi
801046d5:	fc                   	cld
801046d6:	f3 aa                	rep stos %al,%es:(%edi)
801046d8:	8b 7d fc             	mov    -0x4(%ebp),%edi
801046db:	89 d0                	mov    %edx,%eax
801046dd:	c9                   	leave
801046de:	c3                   	ret
801046df:	90                   	nop

801046e0 <memcmp>:

int
memcmp(const void *v1, const void *v2, uint n)
{
801046e0:	55                   	push   %ebp
801046e1:	89 e5                	mov    %esp,%ebp
801046e3:	56                   	push   %esi
801046e4:	8b 75 10             	mov    0x10(%ebp),%esi
801046e7:	8b 45 08             	mov    0x8(%ebp),%eax
801046ea:	53                   	push   %ebx
801046eb:	8b 55 0c             	mov    0xc(%ebp),%edx
  const uchar *s1, *s2;

  s1 = v1;
  s2 = v2;
  while(n-- > 0){
801046ee:	85 f6                	test   %esi,%esi
801046f0:	74 2e                	je     80104720 <memcmp+0x40>
801046f2:	01 c6                	add    %eax,%esi
801046f4:	eb 14                	jmp    8010470a <memcmp+0x2a>
801046f6:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801046fd:	00 
801046fe:	66 90                	xchg   %ax,%ax
    if(*s1 != *s2)
      return *s1 - *s2;
    s1++, s2++;
80104700:	83 c0 01             	add    $0x1,%eax
80104703:	83 c2 01             	add    $0x1,%edx
  while(n-- > 0){
80104706:	39 f0                	cmp    %esi,%eax
80104708:	74 16                	je     80104720 <memcmp+0x40>
    if(*s1 != *s2)
8010470a:	0f b6 08             	movzbl (%eax),%ecx
8010470d:	0f b6 1a             	movzbl (%edx),%ebx
80104710:	38 d9                	cmp    %bl,%cl
80104712:	74 ec                	je     80104700 <memcmp+0x20>
      return *s1 - *s2;
80104714:	0f b6 c1             	movzbl %cl,%eax
80104717:	29 d8                	sub    %ebx,%eax
  }

  return 0;
}
80104719:	5b                   	pop    %ebx
8010471a:	5e                   	pop    %esi
8010471b:	5d                   	pop    %ebp
8010471c:	c3                   	ret
8010471d:	8d 76 00             	lea    0x0(%esi),%esi
80104720:	5b                   	pop    %ebx
  return 0;
80104721:	31 c0                	xor    %eax,%eax
}
80104723:	5e                   	pop    %esi
80104724:	5d                   	pop    %ebp
80104725:	c3                   	ret
80104726:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010472d:	00 
8010472e:	66 90                	xchg   %ax,%ax

80104730 <memmove>:

void*
memmove(void *dst, const void *src, uint n)
{
80104730:	55                   	push   %ebp
80104731:	89 e5                	mov    %esp,%ebp
80104733:	57                   	push   %edi
80104734:	8b 55 08             	mov    0x8(%ebp),%edx
80104737:	8b 45 10             	mov    0x10(%ebp),%eax
8010473a:	56                   	push   %esi
8010473b:	8b 75 0c             	mov    0xc(%ebp),%esi
  const char *s;
  char *d;

  s = src;
  d = dst;
  if(s < d && s + n > d){
8010473e:	39 d6                	cmp    %edx,%esi
80104740:	73 26                	jae    80104768 <memmove+0x38>
80104742:	8d 0c 06             	lea    (%esi,%eax,1),%ecx
80104745:	39 ca                	cmp    %ecx,%edx
80104747:	73 1f                	jae    80104768 <memmove+0x38>
    s += n;
    d += n;
    while(n-- > 0)
80104749:	85 c0                	test   %eax,%eax
8010474b:	74 0f                	je     8010475c <memmove+0x2c>
8010474d:	83 e8 01             	sub    $0x1,%eax
      *--d = *--s;
80104750:	0f b6 0c 06          	movzbl (%esi,%eax,1),%ecx
80104754:	88 0c 02             	mov    %cl,(%edx,%eax,1)
    while(n-- > 0)
80104757:	83 e8 01             	sub    $0x1,%eax
8010475a:	73 f4                	jae    80104750 <memmove+0x20>
  } else
    while(n-- > 0)
      *d++ = *s++;

  return dst;
}
8010475c:	5e                   	pop    %esi
8010475d:	89 d0                	mov    %edx,%eax
8010475f:	5f                   	pop    %edi
80104760:	5d                   	pop    %ebp
80104761:	c3                   	ret
80104762:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    while(n-- > 0)
80104768:	8d 0c 06             	lea    (%esi,%eax,1),%ecx
8010476b:	89 d7                	mov    %edx,%edi
8010476d:	85 c0                	test   %eax,%eax
8010476f:	74 eb                	je     8010475c <memmove+0x2c>
80104771:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
      *d++ = *s++;
80104778:	a4                   	movsb  %ds:(%esi),%es:(%edi)
    while(n-- > 0)
80104779:	39 ce                	cmp    %ecx,%esi
8010477b:	75 fb                	jne    80104778 <memmove+0x48>
}
8010477d:	5e                   	pop    %esi
8010477e:	89 d0                	mov    %edx,%eax
80104780:	5f                   	pop    %edi
80104781:	5d                   	pop    %ebp
80104782:	c3                   	ret
80104783:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010478a:	00 
8010478b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi

80104790 <memcpy>:

// memcpy exists to placate GCC.  Use memmove.
void*
memcpy(void *dst, const void *src, uint n)
{
  return memmove(dst, src, n);
80104790:	eb 9e                	jmp    80104730 <memmove>
80104792:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80104799:	00 
8010479a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi

801047a0 <strncmp>:
}

int
strncmp(const char *p, const char *q, uint n)
{
801047a0:	55                   	push   %ebp
801047a1:	89 e5                	mov    %esp,%ebp
801047a3:	53                   	push   %ebx
801047a4:	8b 55 10             	mov    0x10(%ebp),%edx
801047a7:	8b 45 08             	mov    0x8(%ebp),%eax
801047aa:	8b 4d 0c             	mov    0xc(%ebp),%ecx
  while(n > 0 && *p && *p == *q)
801047ad:	85 d2                	test   %edx,%edx
801047af:	75 16                	jne    801047c7 <strncmp+0x27>
801047b1:	eb 2d                	jmp    801047e0 <strncmp+0x40>
801047b3:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
801047b8:	3a 19                	cmp    (%ecx),%bl
801047ba:	75 12                	jne    801047ce <strncmp+0x2e>
    n--, p++, q++;
801047bc:	83 c0 01             	add    $0x1,%eax
801047bf:	83 c1 01             	add    $0x1,%ecx
  while(n > 0 && *p && *p == *q)
801047c2:	83 ea 01             	sub    $0x1,%edx
801047c5:	74 19                	je     801047e0 <strncmp+0x40>
801047c7:	0f b6 18             	movzbl (%eax),%ebx
801047ca:	84 db                	test   %bl,%bl
801047cc:	75 ea                	jne    801047b8 <strncmp+0x18>
  if(n == 0)
    return 0;
  return (uchar)*p - (uchar)*q;
801047ce:	0f b6 00             	movzbl (%eax),%eax
801047d1:	0f b6 11             	movzbl (%ecx),%edx
}
801047d4:	8b 5d fc             	mov    -0x4(%ebp),%ebx
801047d7:	c9                   	leave
  return (uchar)*p - (uchar)*q;
801047d8:	29 d0                	sub    %edx,%eax
}
801047da:	c3                   	ret
801047db:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
801047e0:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    return 0;
801047e3:	31 c0                	xor    %eax,%eax
}
801047e5:	c9                   	leave
801047e6:	c3                   	ret
801047e7:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801047ee:	00 
801047ef:	90                   	nop

801047f0 <strncpy>:

char*
strncpy(char *s, const char *t, int n)
{
801047f0:	55                   	push   %ebp
801047f1:	89 e5                	mov    %esp,%ebp
801047f3:	57                   	push   %edi
801047f4:	56                   	push   %esi
801047f5:	8b 75 08             	mov    0x8(%ebp),%esi
801047f8:	53                   	push   %ebx
801047f9:	8b 55 10             	mov    0x10(%ebp),%edx
  char *os;

  os = s;
  while(n-- > 0 && (*s++ = *t++) != 0)
801047fc:	89 f0                	mov    %esi,%eax
801047fe:	eb 15                	jmp    80104815 <strncpy+0x25>
80104800:	83 45 0c 01          	addl   $0x1,0xc(%ebp)
80104804:	8b 7d 0c             	mov    0xc(%ebp),%edi
80104807:	83 c0 01             	add    $0x1,%eax
8010480a:	0f b6 4f ff          	movzbl -0x1(%edi),%ecx
8010480e:	88 48 ff             	mov    %cl,-0x1(%eax)
80104811:	84 c9                	test   %cl,%cl
80104813:	74 13                	je     80104828 <strncpy+0x38>
80104815:	89 d3                	mov    %edx,%ebx
80104817:	83 ea 01             	sub    $0x1,%edx
8010481a:	85 db                	test   %ebx,%ebx
8010481c:	7f e2                	jg     80104800 <strncpy+0x10>
    ;
  while(n-- > 0)
    *s++ = 0;
  return os;
}
8010481e:	5b                   	pop    %ebx
8010481f:	89 f0                	mov    %esi,%eax
80104821:	5e                   	pop    %esi
80104822:	5f                   	pop    %edi
80104823:	5d                   	pop    %ebp
80104824:	c3                   	ret
80104825:	8d 76 00             	lea    0x0(%esi),%esi
  while(n-- > 0)
80104828:	8d 0c 18             	lea    (%eax,%ebx,1),%ecx
8010482b:	83 e9 01             	sub    $0x1,%ecx
8010482e:	85 d2                	test   %edx,%edx
80104830:	74 ec                	je     8010481e <strncpy+0x2e>
80104832:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    *s++ = 0;
80104838:	83 c0 01             	add    $0x1,%eax
8010483b:	89 ca                	mov    %ecx,%edx
8010483d:	c6 40 ff 00          	movb   $0x0,-0x1(%eax)
  while(n-- > 0)
80104841:	29 c2                	sub    %eax,%edx
80104843:	85 d2                	test   %edx,%edx
80104845:	7f f1                	jg     80104838 <strncpy+0x48>
}
80104847:	5b                   	pop    %ebx
80104848:	89 f0                	mov    %esi,%eax
8010484a:	5e                   	pop    %esi
8010484b:	5f                   	pop    %edi
8010484c:	5d                   	pop    %ebp
8010484d:	c3                   	ret
8010484e:	66 90                	xchg   %ax,%ax

80104850 <safestrcpy>:

// Like strncpy but guaranteed to NUL-terminate.
char*
safestrcpy(char *s, const char *t, int n)
{
80104850:	55                   	push   %ebp
80104851:	89 e5                	mov    %esp,%ebp
80104853:	56                   	push   %esi
80104854:	8b 55 10             	mov    0x10(%ebp),%edx
80104857:	8b 75 08             	mov    0x8(%ebp),%esi
8010485a:	53                   	push   %ebx
8010485b:	8b 45 0c             	mov    0xc(%ebp),%eax
  char *os;

  os = s;
  if(n <= 0)
8010485e:	85 d2                	test   %edx,%edx
80104860:	7e 25                	jle    80104887 <safestrcpy+0x37>
80104862:	8d 5c 10 ff          	lea    -0x1(%eax,%edx,1),%ebx
80104866:	89 f2                	mov    %esi,%edx
80104868:	eb 16                	jmp    80104880 <safestrcpy+0x30>
8010486a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    return os;
  while(--n > 0 && (*s++ = *t++) != 0)
80104870:	0f b6 08             	movzbl (%eax),%ecx
80104873:	83 c0 01             	add    $0x1,%eax
80104876:	83 c2 01             	add    $0x1,%edx
80104879:	88 4a ff             	mov    %cl,-0x1(%edx)
8010487c:	84 c9                	test   %cl,%cl
8010487e:	74 04                	je     80104884 <safestrcpy+0x34>
80104880:	39 d8                	cmp    %ebx,%eax
80104882:	75 ec                	jne    80104870 <safestrcpy+0x20>
    ;
  *s = 0;
80104884:	c6 02 00             	movb   $0x0,(%edx)
  return os;
}
80104887:	89 f0                	mov    %esi,%eax
80104889:	5b                   	pop    %ebx
8010488a:	5e                   	pop    %esi
8010488b:	5d                   	pop    %ebp
8010488c:	c3                   	ret
8010488d:	8d 76 00             	lea    0x0(%esi),%esi

80104890 <strlen>:

int
strlen(const char *s)
{
80104890:	55                   	push   %ebp
  int n;

  for(n = 0; s[n]; n++)
80104891:	31 c0                	xor    %eax,%eax
{
80104893:	89 e5                	mov    %esp,%ebp
80104895:	8b 55 08             	mov    0x8(%ebp),%edx
  for(n = 0; s[n]; n++)
80104898:	80 3a 00             	cmpb   $0x0,(%edx)
8010489b:	74 0c                	je     801048a9 <strlen+0x19>
8010489d:	8d 76 00             	lea    0x0(%esi),%esi
801048a0:	83 c0 01             	add    $0x1,%eax
801048a3:	80 3c 02 00          	cmpb   $0x0,(%edx,%eax,1)
801048a7:	75 f7                	jne    801048a0 <strlen+0x10>
    ;
  return n;
}
801048a9:	5d                   	pop    %ebp
801048aa:	c3                   	ret

801048ab <swtch>:
# a struct context, and save its address in *old.
# Switch stacks to new and pop previously-saved registers.

.globl swtch
swtch:
  movl 4(%esp), %eax
801048ab:	8b 44 24 04          	mov    0x4(%esp),%eax
  movl 8(%esp), %edx
801048af:	8b 54 24 08          	mov    0x8(%esp),%edx

  # Save old callee-saved registers
  pushl %ebp
801048b3:	55                   	push   %ebp
  pushl %ebx
801048b4:	53                   	push   %ebx
  pushl %esi
801048b5:	56                   	push   %esi
  pushl %edi
801048b6:	57                   	push   %edi

  # Switch stacks
  movl %esp, (%eax)
801048b7:	89 20                	mov    %esp,(%eax)
  movl %edx, %esp
801048b9:	89 d4                	mov    %edx,%esp

  # Load new callee-saved registers
  popl %edi
801048bb:	5f                   	pop    %edi
  popl %esi
801048bc:	5e                   	pop    %esi
  popl %ebx
801048bd:	5b                   	pop    %ebx
  popl %ebp
801048be:	5d                   	pop    %ebp
  ret
801048bf:	c3                   	ret

801048c0 <fetchint>:
// to a saved program counter, and then the first argument.

// Fetch the int at addr from the current process.
int
fetchint(uint addr, int *ip)
{
801048c0:	55                   	push   %ebp
801048c1:	89 e5                	mov    %esp,%ebp
801048c3:	53                   	push   %ebx
801048c4:	83 ec 04             	sub    $0x4,%esp
801048c7:	8b 5d 08             	mov    0x8(%ebp),%ebx
  struct proc *curproc = myproc();
801048ca:	e8 91 f0 ff ff       	call   80103960 <myproc>

  if(addr >= curproc->sz || addr+4 > curproc->sz)
801048cf:	8b 00                	mov    (%eax),%eax
801048d1:	39 c3                	cmp    %eax,%ebx
801048d3:	73 1b                	jae    801048f0 <fetchint+0x30>
801048d5:	8d 53 04             	lea    0x4(%ebx),%edx
801048d8:	39 d0                	cmp    %edx,%eax
801048da:	72 14                	jb     801048f0 <fetchint+0x30>
    return -1;
  *ip = *(int*)(addr);
801048dc:	8b 45 0c             	mov    0xc(%ebp),%eax
801048df:	8b 13                	mov    (%ebx),%edx
801048e1:	89 10                	mov    %edx,(%eax)
  return 0;
801048e3:	31 c0                	xor    %eax,%eax
}
801048e5:	8b 5d fc             	mov    -0x4(%ebp),%ebx
801048e8:	c9                   	leave
801048e9:	c3                   	ret
801048ea:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    return -1;
801048f0:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
801048f5:	eb ee                	jmp    801048e5 <fetchint+0x25>
801048f7:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801048fe:	00 
801048ff:	90                   	nop

80104900 <fetchstr>:
// Fetch the nul-terminated string at addr from the current process.
// Doesn't actually copy the string - just sets *pp to point at it.
// Returns length of string, not including nul.
int
fetchstr(uint addr, char **pp)
{
80104900:	55                   	push   %ebp
80104901:	89 e5                	mov    %esp,%ebp
80104903:	53                   	push   %ebx
80104904:	83 ec 04             	sub    $0x4,%esp
80104907:	8b 5d 08             	mov    0x8(%ebp),%ebx
  char *s, *ep;
  struct proc *curproc = myproc();
8010490a:	e8 51 f0 ff ff       	call   80103960 <myproc>

  if(addr >= curproc->sz)
8010490f:	3b 18                	cmp    (%eax),%ebx
80104911:	73 2d                	jae    80104940 <fetchstr+0x40>
    return -1;
  *pp = (char*)addr;
80104913:	8b 55 0c             	mov    0xc(%ebp),%edx
80104916:	89 1a                	mov    %ebx,(%edx)
  ep = (char*)curproc->sz;
80104918:	8b 10                	mov    (%eax),%edx
  for(s = *pp; s < ep; s++){
8010491a:	39 d3                	cmp    %edx,%ebx
8010491c:	73 22                	jae    80104940 <fetchstr+0x40>
8010491e:	89 d8                	mov    %ebx,%eax
80104920:	eb 0d                	jmp    8010492f <fetchstr+0x2f>
80104922:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
80104928:	83 c0 01             	add    $0x1,%eax
8010492b:	39 d0                	cmp    %edx,%eax
8010492d:	73 11                	jae    80104940 <fetchstr+0x40>
    if(*s == 0)
8010492f:	80 38 00             	cmpb   $0x0,(%eax)
80104932:	75 f4                	jne    80104928 <fetchstr+0x28>
      return s - *pp;
80104934:	29 d8                	sub    %ebx,%eax
  }
  return -1;
}
80104936:	8b 5d fc             	mov    -0x4(%ebp),%ebx
80104939:	c9                   	leave
8010493a:	c3                   	ret
8010493b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
80104940:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    return -1;
80104943:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
}
80104948:	c9                   	leave
80104949:	c3                   	ret
8010494a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi

80104950 <argint>:

// Fetch the nth 32-bit system call argument.
int
argint(int n, int *ip)
{
80104950:	55                   	push   %ebp
80104951:	89 e5                	mov    %esp,%ebp
80104953:	56                   	push   %esi
80104954:	53                   	push   %ebx
  return fetchint((myproc()->tf->esp) + 4 + 4*n, ip);
80104955:	e8 06 f0 ff ff       	call   80103960 <myproc>
8010495a:	8b 55 08             	mov    0x8(%ebp),%edx
8010495d:	8b 40 18             	mov    0x18(%eax),%eax
80104960:	8b 40 44             	mov    0x44(%eax),%eax
80104963:	8d 1c 90             	lea    (%eax,%edx,4),%ebx
  struct proc *curproc = myproc();
80104966:	e8 f5 ef ff ff       	call   80103960 <myproc>
  return fetchint((myproc()->tf->esp) + 4 + 4*n, ip);
8010496b:	8d 73 04             	lea    0x4(%ebx),%esi
  if(addr >= curproc->sz || addr+4 > curproc->sz)
8010496e:	8b 00                	mov    (%eax),%eax
80104970:	39 c6                	cmp    %eax,%esi
80104972:	73 1c                	jae    80104990 <argint+0x40>
80104974:	8d 53 08             	lea    0x8(%ebx),%edx
80104977:	39 d0                	cmp    %edx,%eax
80104979:	72 15                	jb     80104990 <argint+0x40>
  *ip = *(int*)(addr);
8010497b:	8b 45 0c             	mov    0xc(%ebp),%eax
8010497e:	8b 53 04             	mov    0x4(%ebx),%edx
80104981:	89 10                	mov    %edx,(%eax)
  return 0;
80104983:	31 c0                	xor    %eax,%eax
}
80104985:	5b                   	pop    %ebx
80104986:	5e                   	pop    %esi
80104987:	5d                   	pop    %ebp
80104988:	c3                   	ret
80104989:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    return -1;
80104990:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
  return fetchint((myproc()->tf->esp) + 4 + 4*n, ip);
80104995:	eb ee                	jmp    80104985 <argint+0x35>
80104997:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010499e:	00 
8010499f:	90                   	nop

801049a0 <argptr>:
// Fetch the nth word-sized system call argument as a pointer
// to a block of memory of size bytes.  Check that the pointer
// lies within the process address space.
int
argptr(int n, char **pp, int size)
{
801049a0:	55                   	push   %ebp
801049a1:	89 e5                	mov    %esp,%ebp
801049a3:	57                   	push   %edi
801049a4:	56                   	push   %esi
801049a5:	53                   	push   %ebx
801049a6:	83 ec 0c             	sub    $0xc,%esp
  int i;
  struct proc *curproc = myproc();
801049a9:	e8 b2 ef ff ff       	call   80103960 <myproc>
801049ae:	89 c6                	mov    %eax,%esi
  return fetchint((myproc()->tf->esp) + 4 + 4*n, ip);
801049b0:	e8 ab ef ff ff       	call   80103960 <myproc>
801049b5:	8b 55 08             	mov    0x8(%ebp),%edx
801049b8:	8b 40 18             	mov    0x18(%eax),%eax
801049bb:	8b 40 44             	mov    0x44(%eax),%eax
801049be:	8d 1c 90             	lea    (%eax,%edx,4),%ebx
  struct proc *curproc = myproc();
801049c1:	e8 9a ef ff ff       	call   80103960 <myproc>
  return fetchint((myproc()->tf->esp) + 4 + 4*n, ip);
801049c6:	8d 7b 04             	lea    0x4(%ebx),%edi
  if(addr >= curproc->sz || addr+4 > curproc->sz)
801049c9:	8b 00                	mov    (%eax),%eax
801049cb:	39 c7                	cmp    %eax,%edi
801049cd:	73 31                	jae    80104a00 <argptr+0x60>
801049cf:	8d 4b 08             	lea    0x8(%ebx),%ecx
801049d2:	39 c8                	cmp    %ecx,%eax
801049d4:	72 2a                	jb     80104a00 <argptr+0x60>
 
  if(argint(n, &i) < 0)
    return -1;
  if(size < 0 || (uint)i >= curproc->sz || (uint)i+size > curproc->sz)
801049d6:	8b 55 10             	mov    0x10(%ebp),%edx
  *ip = *(int*)(addr);
801049d9:	8b 43 04             	mov    0x4(%ebx),%eax
  if(size < 0 || (uint)i >= curproc->sz || (uint)i+size > curproc->sz)
801049dc:	85 d2                	test   %edx,%edx
801049de:	78 20                	js     80104a00 <argptr+0x60>
801049e0:	8b 16                	mov    (%esi),%edx
801049e2:	39 d0                	cmp    %edx,%eax
801049e4:	73 1a                	jae    80104a00 <argptr+0x60>
801049e6:	8b 5d 10             	mov    0x10(%ebp),%ebx
801049e9:	01 c3                	add    %eax,%ebx
801049eb:	39 da                	cmp    %ebx,%edx
801049ed:	72 11                	jb     80104a00 <argptr+0x60>
    return -1;
  *pp = (char*)i;
801049ef:	8b 55 0c             	mov    0xc(%ebp),%edx
801049f2:	89 02                	mov    %eax,(%edx)
  return 0;
801049f4:	31 c0                	xor    %eax,%eax
}
801049f6:	83 c4 0c             	add    $0xc,%esp
801049f9:	5b                   	pop    %ebx
801049fa:	5e                   	pop    %esi
801049fb:	5f                   	pop    %edi
801049fc:	5d                   	pop    %ebp
801049fd:	c3                   	ret
801049fe:	66 90                	xchg   %ax,%ax
    return -1;
80104a00:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
80104a05:	eb ef                	jmp    801049f6 <argptr+0x56>
80104a07:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80104a0e:	00 
80104a0f:	90                   	nop

80104a10 <argstr>:
// Check that the pointer is valid and the string is nul-terminated.
// (There is no shared writable memory, so the string can't change
// between this check and being used by the kernel.)
int
argstr(int n, char **pp)
{
80104a10:	55                   	push   %ebp
80104a11:	89 e5                	mov    %esp,%ebp
80104a13:	56                   	push   %esi
80104a14:	53                   	push   %ebx
  return fetchint((myproc()->tf->esp) + 4 + 4*n, ip);
80104a15:	e8 46 ef ff ff       	call   80103960 <myproc>
80104a1a:	8b 55 08             	mov    0x8(%ebp),%edx
80104a1d:	8b 40 18             	mov    0x18(%eax),%eax
80104a20:	8b 40 44             	mov    0x44(%eax),%eax
80104a23:	8d 1c 90             	lea    (%eax,%edx,4),%ebx
  struct proc *curproc = myproc();
80104a26:	e8 35 ef ff ff       	call   80103960 <myproc>
  return fetchint((myproc()->tf->esp) + 4 + 4*n, ip);
80104a2b:	8d 73 04             	lea    0x4(%ebx),%esi
  if(addr >= curproc->sz || addr+4 > curproc->sz)
80104a2e:	8b 00                	mov    (%eax),%eax
80104a30:	39 c6                	cmp    %eax,%esi
80104a32:	73 44                	jae    80104a78 <argstr+0x68>
80104a34:	8d 53 08             	lea    0x8(%ebx),%edx
80104a37:	39 d0                	cmp    %edx,%eax
80104a39:	72 3d                	jb     80104a78 <argstr+0x68>
  *ip = *(int*)(addr);
80104a3b:	8b 5b 04             	mov    0x4(%ebx),%ebx
  struct proc *curproc = myproc();
80104a3e:	e8 1d ef ff ff       	call   80103960 <myproc>
  if(addr >= curproc->sz)
80104a43:	3b 18                	cmp    (%eax),%ebx
80104a45:	73 31                	jae    80104a78 <argstr+0x68>
  *pp = (char*)addr;
80104a47:	8b 55 0c             	mov    0xc(%ebp),%edx
80104a4a:	89 1a                	mov    %ebx,(%edx)
  ep = (char*)curproc->sz;
80104a4c:	8b 10                	mov    (%eax),%edx
  for(s = *pp; s < ep; s++){
80104a4e:	39 d3                	cmp    %edx,%ebx
80104a50:	73 26                	jae    80104a78 <argstr+0x68>
80104a52:	89 d8                	mov    %ebx,%eax
80104a54:	eb 11                	jmp    80104a67 <argstr+0x57>
80104a56:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80104a5d:	00 
80104a5e:	66 90                	xchg   %ax,%ax
80104a60:	83 c0 01             	add    $0x1,%eax
80104a63:	39 d0                	cmp    %edx,%eax
80104a65:	73 11                	jae    80104a78 <argstr+0x68>
    if(*s == 0)
80104a67:	80 38 00             	cmpb   $0x0,(%eax)
80104a6a:	75 f4                	jne    80104a60 <argstr+0x50>
      return s - *pp;
80104a6c:	29 d8                	sub    %ebx,%eax
  int addr;
  if(argint(n, &addr) < 0)
    return -1;
  return fetchstr(addr, pp);
}
80104a6e:	5b                   	pop    %ebx
80104a6f:	5e                   	pop    %esi
80104a70:	5d                   	pop    %ebp
80104a71:	c3                   	ret
80104a72:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
80104a78:	5b                   	pop    %ebx
    return -1;
80104a79:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
}
80104a7e:	5e                   	pop    %esi
80104a7f:	5d                   	pop    %ebp
80104a80:	c3                   	ret
80104a81:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80104a88:	00 
80104a89:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi

80104a90 <syscall>:
[SYS_close]   sys_close,
};

void
syscall(void)
{
80104a90:	55                   	push   %ebp
80104a91:	89 e5                	mov    %esp,%ebp
80104a93:	53                   	push   %ebx
80104a94:	83 ec 04             	sub    $0x4,%esp
  int num;
  struct proc *curproc = myproc();
80104a97:	e8 c4 ee ff ff       	call   80103960 <myproc>
80104a9c:	89 c3                	mov    %eax,%ebx

  num = curproc->tf->eax;
80104a9e:	8b 40 18             	mov    0x18(%eax),%eax
80104aa1:	8b 40 1c             	mov    0x1c(%eax),%eax
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
80104aa4:	8d 50 ff             	lea    -0x1(%eax),%edx
80104aa7:	83 fa 14             	cmp    $0x14,%edx
80104aaa:	77 24                	ja     80104ad0 <syscall+0x40>
80104aac:	8b 14 85 60 7a 10 80 	mov    -0x7fef85a0(,%eax,4),%edx
80104ab3:	85 d2                	test   %edx,%edx
80104ab5:	74 19                	je     80104ad0 <syscall+0x40>
    curproc->tf->eax = syscalls[num]();
80104ab7:	ff d2                	call   *%edx
80104ab9:	89 c2                	mov    %eax,%edx
80104abb:	8b 43 18             	mov    0x18(%ebx),%eax
80104abe:	89 50 1c             	mov    %edx,0x1c(%eax)
  } else {
    cprintf("%d %s: unknown sys call %d\n",
            curproc->pid, curproc->name, num);
    curproc->tf->eax = -1;
  }
}
80104ac1:	8b 5d fc             	mov    -0x4(%ebp),%ebx
80104ac4:	c9                   	leave
80104ac5:	c3                   	ret
80104ac6:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80104acd:	00 
80104ace:	66 90                	xchg   %ax,%ax
    cprintf("%d %s: unknown sys call %d\n",
80104ad0:	50                   	push   %eax
            curproc->pid, curproc->name, num);
80104ad1:	8d 43 6c             	lea    0x6c(%ebx),%eax
    cprintf("%d %s: unknown sys call %d\n",
80104ad4:	50                   	push   %eax
80104ad5:	ff 73 10             	push   0x10(%ebx)
80104ad8:	68 d1 74 10 80       	push   $0x801074d1
80104add:	e8 ce bb ff ff       	call   801006b0 <cprintf>
    curproc->tf->eax = -1;
80104ae2:	8b 43 18             	mov    0x18(%ebx),%eax
80104ae5:	83 c4 10             	add    $0x10,%esp
80104ae8:	c7 40 1c ff ff ff ff 	movl   $0xffffffff,0x1c(%eax)
}
80104aef:	8b 5d fc             	mov    -0x4(%ebp),%ebx
80104af2:	c9                   	leave
80104af3:	c3                   	ret
80104af4:	66 90                	xchg   %ax,%ax
80104af6:	66 90                	xchg   %ax,%ax
80104af8:	66 90                	xchg   %ax,%ax
80104afa:	66 90                	xchg   %ax,%ax
80104afc:	66 90                	xchg   %ax,%ax
80104afe:	66 90                	xchg   %ax,%ax

80104b00 <create>:
  return -1;
}

static struct inode*
create(char *path, short type, short major, short minor)
{
80104b00:	55                   	push   %ebp
80104b01:	89 e5                	mov    %esp,%ebp
80104b03:	57                   	push   %edi
80104b04:	56                   	push   %esi
  struct inode *ip, *dp;
  char name[DIRSIZ];

  if((dp = nameiparent(path, name)) == 0)
80104b05:	8d 7d da             	lea    -0x26(%ebp),%edi
{
80104b08:	53                   	push   %ebx
80104b09:	83 ec 34             	sub    $0x34,%esp
80104b0c:	89 4d d0             	mov    %ecx,-0x30(%ebp)
80104b0f:	8b 4d 08             	mov    0x8(%ebp),%ecx
80104b12:	89 55 d4             	mov    %edx,-0x2c(%ebp)
80104b15:	89 4d cc             	mov    %ecx,-0x34(%ebp)
  if((dp = nameiparent(path, name)) == 0)
80104b18:	57                   	push   %edi
80104b19:	50                   	push   %eax
80104b1a:	e8 81 d5 ff ff       	call   801020a0 <nameiparent>
80104b1f:	83 c4 10             	add    $0x10,%esp
80104b22:	85 c0                	test   %eax,%eax
80104b24:	74 5e                	je     80104b84 <create+0x84>
    return 0;
  ilock(dp);
80104b26:	83 ec 0c             	sub    $0xc,%esp
80104b29:	89 c3                	mov    %eax,%ebx
80104b2b:	50                   	push   %eax
80104b2c:	e8 6f cc ff ff       	call   801017a0 <ilock>

  if((ip = dirlookup(dp, name, 0)) != 0){
80104b31:	83 c4 0c             	add    $0xc,%esp
80104b34:	6a 00                	push   $0x0
80104b36:	57                   	push   %edi
80104b37:	53                   	push   %ebx
80104b38:	e8 b3 d1 ff ff       	call   80101cf0 <dirlookup>
80104b3d:	83 c4 10             	add    $0x10,%esp
80104b40:	89 c6                	mov    %eax,%esi
80104b42:	85 c0                	test   %eax,%eax
80104b44:	74 4a                	je     80104b90 <create+0x90>
    iunlockput(dp);
80104b46:	83 ec 0c             	sub    $0xc,%esp
80104b49:	53                   	push   %ebx
80104b4a:	e8 e1 ce ff ff       	call   80101a30 <iunlockput>
    ilock(ip);
80104b4f:	89 34 24             	mov    %esi,(%esp)
80104b52:	e8 49 cc ff ff       	call   801017a0 <ilock>
    if(type == T_FILE && ip->type == T_FILE)
80104b57:	83 c4 10             	add    $0x10,%esp
80104b5a:	66 83 7d d4 02       	cmpw   $0x2,-0x2c(%ebp)
80104b5f:	75 17                	jne    80104b78 <create+0x78>
80104b61:	66 83 7e 50 02       	cmpw   $0x2,0x50(%esi)
80104b66:	75 10                	jne    80104b78 <create+0x78>
    panic("create: dirlink");

  iunlockput(dp);

  return ip;
}
80104b68:	8d 65 f4             	lea    -0xc(%ebp),%esp
80104b6b:	89 f0                	mov    %esi,%eax
80104b6d:	5b                   	pop    %ebx
80104b6e:	5e                   	pop    %esi
80104b6f:	5f                   	pop    %edi
80104b70:	5d                   	pop    %ebp
80104b71:	c3                   	ret
80104b72:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    iunlockput(ip);
80104b78:	83 ec 0c             	sub    $0xc,%esp
80104b7b:	56                   	push   %esi
80104b7c:	e8 af ce ff ff       	call   80101a30 <iunlockput>
    return 0;
80104b81:	83 c4 10             	add    $0x10,%esp
}
80104b84:	8d 65 f4             	lea    -0xc(%ebp),%esp
    return 0;
80104b87:	31 f6                	xor    %esi,%esi
}
80104b89:	5b                   	pop    %ebx
80104b8a:	89 f0                	mov    %esi,%eax
80104b8c:	5e                   	pop    %esi
80104b8d:	5f                   	pop    %edi
80104b8e:	5d                   	pop    %ebp
80104b8f:	c3                   	ret
  if((ip = ialloc(dp->dev, type)) == 0)
80104b90:	0f bf 45 d4          	movswl -0x2c(%ebp),%eax
80104b94:	83 ec 08             	sub    $0x8,%esp
80104b97:	50                   	push   %eax
80104b98:	ff 33                	push   (%ebx)
80104b9a:	e8 91 ca ff ff       	call   80101630 <ialloc>
80104b9f:	83 c4 10             	add    $0x10,%esp
80104ba2:	89 c6                	mov    %eax,%esi
80104ba4:	85 c0                	test   %eax,%eax
80104ba6:	0f 84 bc 00 00 00    	je     80104c68 <create+0x168>
  ilock(ip);
80104bac:	83 ec 0c             	sub    $0xc,%esp
80104baf:	50                   	push   %eax
80104bb0:	e8 eb cb ff ff       	call   801017a0 <ilock>
  ip->major = major;
80104bb5:	0f b7 45 d0          	movzwl -0x30(%ebp),%eax
80104bb9:	66 89 46 52          	mov    %ax,0x52(%esi)
  ip->minor = minor;
80104bbd:	0f b7 45 cc          	movzwl -0x34(%ebp),%eax
80104bc1:	66 89 46 54          	mov    %ax,0x54(%esi)
  ip->nlink = 1;
80104bc5:	b8 01 00 00 00       	mov    $0x1,%eax
80104bca:	66 89 46 56          	mov    %ax,0x56(%esi)
  iupdate(ip);
80104bce:	89 34 24             	mov    %esi,(%esp)
80104bd1:	e8 1a cb ff ff       	call   801016f0 <iupdate>
  if(type == T_DIR){  // Create . and .. entries.
80104bd6:	83 c4 10             	add    $0x10,%esp
80104bd9:	66 83 7d d4 01       	cmpw   $0x1,-0x2c(%ebp)
80104bde:	74 30                	je     80104c10 <create+0x110>
  if(dirlink(dp, name, ip->inum) < 0)
80104be0:	83 ec 04             	sub    $0x4,%esp
80104be3:	ff 76 04             	push   0x4(%esi)
80104be6:	57                   	push   %edi
80104be7:	53                   	push   %ebx
80104be8:	e8 d3 d3 ff ff       	call   80101fc0 <dirlink>
80104bed:	83 c4 10             	add    $0x10,%esp
80104bf0:	85 c0                	test   %eax,%eax
80104bf2:	78 67                	js     80104c5b <create+0x15b>
  iunlockput(dp);
80104bf4:	83 ec 0c             	sub    $0xc,%esp
80104bf7:	53                   	push   %ebx
80104bf8:	e8 33 ce ff ff       	call   80101a30 <iunlockput>
  return ip;
80104bfd:	83 c4 10             	add    $0x10,%esp
}
80104c00:	8d 65 f4             	lea    -0xc(%ebp),%esp
80104c03:	89 f0                	mov    %esi,%eax
80104c05:	5b                   	pop    %ebx
80104c06:	5e                   	pop    %esi
80104c07:	5f                   	pop    %edi
80104c08:	5d                   	pop    %ebp
80104c09:	c3                   	ret
80104c0a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    iupdate(dp);
80104c10:	83 ec 0c             	sub    $0xc,%esp
    dp->nlink++;  // for ".."
80104c13:	66 83 43 56 01       	addw   $0x1,0x56(%ebx)
    iupdate(dp);
80104c18:	53                   	push   %ebx
80104c19:	e8 d2 ca ff ff       	call   801016f0 <iupdate>
    if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
80104c1e:	83 c4 0c             	add    $0xc,%esp
80104c21:	ff 76 04             	push   0x4(%esi)
80104c24:	68 09 75 10 80       	push   $0x80107509
80104c29:	56                   	push   %esi
80104c2a:	e8 91 d3 ff ff       	call   80101fc0 <dirlink>
80104c2f:	83 c4 10             	add    $0x10,%esp
80104c32:	85 c0                	test   %eax,%eax
80104c34:	78 18                	js     80104c4e <create+0x14e>
80104c36:	83 ec 04             	sub    $0x4,%esp
80104c39:	ff 73 04             	push   0x4(%ebx)
80104c3c:	68 08 75 10 80       	push   $0x80107508
80104c41:	56                   	push   %esi
80104c42:	e8 79 d3 ff ff       	call   80101fc0 <dirlink>
80104c47:	83 c4 10             	add    $0x10,%esp
80104c4a:	85 c0                	test   %eax,%eax
80104c4c:	79 92                	jns    80104be0 <create+0xe0>
      panic("create dots");
80104c4e:	83 ec 0c             	sub    $0xc,%esp
80104c51:	68 fc 74 10 80       	push   $0x801074fc
80104c56:	e8 25 b7 ff ff       	call   80100380 <panic>
    panic("create: dirlink");
80104c5b:	83 ec 0c             	sub    $0xc,%esp
80104c5e:	68 0b 75 10 80       	push   $0x8010750b
80104c63:	e8 18 b7 ff ff       	call   80100380 <panic>
    panic("create: ialloc");
80104c68:	83 ec 0c             	sub    $0xc,%esp
80104c6b:	68 ed 74 10 80       	push   $0x801074ed
80104c70:	e8 0b b7 ff ff       	call   80100380 <panic>
80104c75:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80104c7c:	00 
80104c7d:	8d 76 00             	lea    0x0(%esi),%esi

80104c80 <sys_dup>:
{
80104c80:	55                   	push   %ebp
80104c81:	89 e5                	mov    %esp,%ebp
80104c83:	56                   	push   %esi
80104c84:	53                   	push   %ebx
  if(argint(n, &fd) < 0)
80104c85:	8d 45 f4             	lea    -0xc(%ebp),%eax
{
80104c88:	83 ec 18             	sub    $0x18,%esp
  if(argint(n, &fd) < 0)
80104c8b:	50                   	push   %eax
80104c8c:	6a 00                	push   $0x0
80104c8e:	e8 bd fc ff ff       	call   80104950 <argint>
80104c93:	83 c4 10             	add    $0x10,%esp
80104c96:	85 c0                	test   %eax,%eax
80104c98:	78 36                	js     80104cd0 <sys_dup+0x50>
  if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
80104c9a:	83 7d f4 0f          	cmpl   $0xf,-0xc(%ebp)
80104c9e:	77 30                	ja     80104cd0 <sys_dup+0x50>
80104ca0:	e8 bb ec ff ff       	call   80103960 <myproc>
80104ca5:	8b 55 f4             	mov    -0xc(%ebp),%edx
80104ca8:	8b 74 90 28          	mov    0x28(%eax,%edx,4),%esi
80104cac:	85 f6                	test   %esi,%esi
80104cae:	74 20                	je     80104cd0 <sys_dup+0x50>
  struct proc *curproc = myproc();
80104cb0:	e8 ab ec ff ff       	call   80103960 <myproc>
  for(fd = 0; fd < NOFILE; fd++){
80104cb5:	31 db                	xor    %ebx,%ebx
80104cb7:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80104cbe:	00 
80104cbf:	90                   	nop
    if(curproc->ofile[fd] == 0){
80104cc0:	8b 54 98 28          	mov    0x28(%eax,%ebx,4),%edx
80104cc4:	85 d2                	test   %edx,%edx
80104cc6:	74 18                	je     80104ce0 <sys_dup+0x60>
  for(fd = 0; fd < NOFILE; fd++){
80104cc8:	83 c3 01             	add    $0x1,%ebx
80104ccb:	83 fb 10             	cmp    $0x10,%ebx
80104cce:	75 f0                	jne    80104cc0 <sys_dup+0x40>
}
80104cd0:	8d 65 f8             	lea    -0x8(%ebp),%esp
    return -1;
80104cd3:	bb ff ff ff ff       	mov    $0xffffffff,%ebx
}
80104cd8:	89 d8                	mov    %ebx,%eax
80104cda:	5b                   	pop    %ebx
80104cdb:	5e                   	pop    %esi
80104cdc:	5d                   	pop    %ebp
80104cdd:	c3                   	ret
80104cde:	66 90                	xchg   %ax,%ax
  filedup(f);
80104ce0:	83 ec 0c             	sub    $0xc,%esp
      curproc->ofile[fd] = f;
80104ce3:	89 74 98 28          	mov    %esi,0x28(%eax,%ebx,4)
  filedup(f);
80104ce7:	56                   	push   %esi
80104ce8:	e8 d3 c1 ff ff       	call   80100ec0 <filedup>
  return fd;
80104ced:	83 c4 10             	add    $0x10,%esp
}
80104cf0:	8d 65 f8             	lea    -0x8(%ebp),%esp
80104cf3:	89 d8                	mov    %ebx,%eax
80104cf5:	5b                   	pop    %ebx
80104cf6:	5e                   	pop    %esi
80104cf7:	5d                   	pop    %ebp
80104cf8:	c3                   	ret
80104cf9:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi

80104d00 <sys_read>:
{
80104d00:	55                   	push   %ebp
80104d01:	89 e5                	mov    %esp,%ebp
80104d03:	56                   	push   %esi
80104d04:	53                   	push   %ebx
  if(argint(n, &fd) < 0)
80104d05:	8d 5d f4             	lea    -0xc(%ebp),%ebx
{
80104d08:	83 ec 18             	sub    $0x18,%esp
  if(argint(n, &fd) < 0)
80104d0b:	53                   	push   %ebx
80104d0c:	6a 00                	push   $0x0
80104d0e:	e8 3d fc ff ff       	call   80104950 <argint>
80104d13:	83 c4 10             	add    $0x10,%esp
80104d16:	85 c0                	test   %eax,%eax
80104d18:	78 5e                	js     80104d78 <sys_read+0x78>
  if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
80104d1a:	83 7d f4 0f          	cmpl   $0xf,-0xc(%ebp)
80104d1e:	77 58                	ja     80104d78 <sys_read+0x78>
80104d20:	e8 3b ec ff ff       	call   80103960 <myproc>
80104d25:	8b 55 f4             	mov    -0xc(%ebp),%edx
80104d28:	8b 74 90 28          	mov    0x28(%eax,%edx,4),%esi
80104d2c:	85 f6                	test   %esi,%esi
80104d2e:	74 48                	je     80104d78 <sys_read+0x78>
  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
80104d30:	83 ec 08             	sub    $0x8,%esp
80104d33:	8d 45 f0             	lea    -0x10(%ebp),%eax
80104d36:	50                   	push   %eax
80104d37:	6a 02                	push   $0x2
80104d39:	e8 12 fc ff ff       	call   80104950 <argint>
80104d3e:	83 c4 10             	add    $0x10,%esp
80104d41:	85 c0                	test   %eax,%eax
80104d43:	78 33                	js     80104d78 <sys_read+0x78>
80104d45:	83 ec 04             	sub    $0x4,%esp
80104d48:	ff 75 f0             	push   -0x10(%ebp)
80104d4b:	53                   	push   %ebx
80104d4c:	6a 01                	push   $0x1
80104d4e:	e8 4d fc ff ff       	call   801049a0 <argptr>
80104d53:	83 c4 10             	add    $0x10,%esp
80104d56:	85 c0                	test   %eax,%eax
80104d58:	78 1e                	js     80104d78 <sys_read+0x78>
  return fileread(f, p, n);
80104d5a:	83 ec 04             	sub    $0x4,%esp
80104d5d:	ff 75 f0             	push   -0x10(%ebp)
80104d60:	ff 75 f4             	push   -0xc(%ebp)
80104d63:	56                   	push   %esi
80104d64:	e8 d7 c2 ff ff       	call   80101040 <fileread>
80104d69:	83 c4 10             	add    $0x10,%esp
}
80104d6c:	8d 65 f8             	lea    -0x8(%ebp),%esp
80104d6f:	5b                   	pop    %ebx
80104d70:	5e                   	pop    %esi
80104d71:	5d                   	pop    %ebp
80104d72:	c3                   	ret
80104d73:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
    return -1;
80104d78:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
80104d7d:	eb ed                	jmp    80104d6c <sys_read+0x6c>
80104d7f:	90                   	nop

80104d80 <sys_write>:
{
80104d80:	55                   	push   %ebp
80104d81:	89 e5                	mov    %esp,%ebp
80104d83:	56                   	push   %esi
80104d84:	53                   	push   %ebx
  if(argint(n, &fd) < 0)
80104d85:	8d 5d f4             	lea    -0xc(%ebp),%ebx
{
80104d88:	83 ec 18             	sub    $0x18,%esp
  if(argint(n, &fd) < 0)
80104d8b:	53                   	push   %ebx
80104d8c:	6a 00                	push   $0x0
80104d8e:	e8 bd fb ff ff       	call   80104950 <argint>
80104d93:	83 c4 10             	add    $0x10,%esp
80104d96:	85 c0                	test   %eax,%eax
80104d98:	78 5e                	js     80104df8 <sys_write+0x78>
  if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
80104d9a:	83 7d f4 0f          	cmpl   $0xf,-0xc(%ebp)
80104d9e:	77 58                	ja     80104df8 <sys_write+0x78>
80104da0:	e8 bb eb ff ff       	call   80103960 <myproc>
80104da5:	8b 55 f4             	mov    -0xc(%ebp),%edx
80104da8:	8b 74 90 28          	mov    0x28(%eax,%edx,4),%esi
80104dac:	85 f6                	test   %esi,%esi
80104dae:	74 48                	je     80104df8 <sys_write+0x78>
  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
80104db0:	83 ec 08             	sub    $0x8,%esp
80104db3:	8d 45 f0             	lea    -0x10(%ebp),%eax
80104db6:	50                   	push   %eax
80104db7:	6a 02                	push   $0x2
80104db9:	e8 92 fb ff ff       	call   80104950 <argint>
80104dbe:	83 c4 10             	add    $0x10,%esp
80104dc1:	85 c0                	test   %eax,%eax
80104dc3:	78 33                	js     80104df8 <sys_write+0x78>
80104dc5:	83 ec 04             	sub    $0x4,%esp
80104dc8:	ff 75 f0             	push   -0x10(%ebp)
80104dcb:	53                   	push   %ebx
80104dcc:	6a 01                	push   $0x1
80104dce:	e8 cd fb ff ff       	call   801049a0 <argptr>
80104dd3:	83 c4 10             	add    $0x10,%esp
80104dd6:	85 c0                	test   %eax,%eax
80104dd8:	78 1e                	js     80104df8 <sys_write+0x78>
  return filewrite(f, p, n);
80104dda:	83 ec 04             	sub    $0x4,%esp
80104ddd:	ff 75 f0             	push   -0x10(%ebp)
80104de0:	ff 75 f4             	push   -0xc(%ebp)
80104de3:	56                   	push   %esi
80104de4:	e8 e7 c2 ff ff       	call   801010d0 <filewrite>
80104de9:	83 c4 10             	add    $0x10,%esp
}
80104dec:	8d 65 f8             	lea    -0x8(%ebp),%esp
80104def:	5b                   	pop    %ebx
80104df0:	5e                   	pop    %esi
80104df1:	5d                   	pop    %ebp
80104df2:	c3                   	ret
80104df3:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
    return -1;
80104df8:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
80104dfd:	eb ed                	jmp    80104dec <sys_write+0x6c>
80104dff:	90                   	nop

80104e00 <sys_close>:
{
80104e00:	55                   	push   %ebp
80104e01:	89 e5                	mov    %esp,%ebp
80104e03:	56                   	push   %esi
80104e04:	53                   	push   %ebx
  if(argint(n, &fd) < 0)
80104e05:	8d 45 f4             	lea    -0xc(%ebp),%eax
{
80104e08:	83 ec 18             	sub    $0x18,%esp
  if(argint(n, &fd) < 0)
80104e0b:	50                   	push   %eax
80104e0c:	6a 00                	push   $0x0
80104e0e:	e8 3d fb ff ff       	call   80104950 <argint>
80104e13:	83 c4 10             	add    $0x10,%esp
80104e16:	85 c0                	test   %eax,%eax
80104e18:	78 3e                	js     80104e58 <sys_close+0x58>
  if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
80104e1a:	83 7d f4 0f          	cmpl   $0xf,-0xc(%ebp)
80104e1e:	77 38                	ja     80104e58 <sys_close+0x58>
80104e20:	e8 3b eb ff ff       	call   80103960 <myproc>
80104e25:	8b 55 f4             	mov    -0xc(%ebp),%edx
80104e28:	8d 5a 08             	lea    0x8(%edx),%ebx
80104e2b:	8b 74 98 08          	mov    0x8(%eax,%ebx,4),%esi
80104e2f:	85 f6                	test   %esi,%esi
80104e31:	74 25                	je     80104e58 <sys_close+0x58>
  myproc()->ofile[fd] = 0;
80104e33:	e8 28 eb ff ff       	call   80103960 <myproc>
  fileclose(f);
80104e38:	83 ec 0c             	sub    $0xc,%esp
  myproc()->ofile[fd] = 0;
80104e3b:	c7 44 98 08 00 00 00 	movl   $0x0,0x8(%eax,%ebx,4)
80104e42:	00 
  fileclose(f);
80104e43:	56                   	push   %esi
80104e44:	e8 c7 c0 ff ff       	call   80100f10 <fileclose>
  return 0;
80104e49:	83 c4 10             	add    $0x10,%esp
80104e4c:	31 c0                	xor    %eax,%eax
}
80104e4e:	8d 65 f8             	lea    -0x8(%ebp),%esp
80104e51:	5b                   	pop    %ebx
80104e52:	5e                   	pop    %esi
80104e53:	5d                   	pop    %ebp
80104e54:	c3                   	ret
80104e55:	8d 76 00             	lea    0x0(%esi),%esi
    return -1;
80104e58:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
80104e5d:	eb ef                	jmp    80104e4e <sys_close+0x4e>
80104e5f:	90                   	nop

80104e60 <sys_fstat>:
{
80104e60:	55                   	push   %ebp
80104e61:	89 e5                	mov    %esp,%ebp
80104e63:	56                   	push   %esi
80104e64:	53                   	push   %ebx
  if(argint(n, &fd) < 0)
80104e65:	8d 5d f4             	lea    -0xc(%ebp),%ebx
{
80104e68:	83 ec 18             	sub    $0x18,%esp
  if(argint(n, &fd) < 0)
80104e6b:	53                   	push   %ebx
80104e6c:	6a 00                	push   $0x0
80104e6e:	e8 dd fa ff ff       	call   80104950 <argint>
80104e73:	83 c4 10             	add    $0x10,%esp
80104e76:	85 c0                	test   %eax,%eax
80104e78:	78 46                	js     80104ec0 <sys_fstat+0x60>
  if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
80104e7a:	83 7d f4 0f          	cmpl   $0xf,-0xc(%ebp)
80104e7e:	77 40                	ja     80104ec0 <sys_fstat+0x60>
80104e80:	e8 db ea ff ff       	call   80103960 <myproc>
80104e85:	8b 55 f4             	mov    -0xc(%ebp),%edx
80104e88:	8b 74 90 28          	mov    0x28(%eax,%edx,4),%esi
80104e8c:	85 f6                	test   %esi,%esi
80104e8e:	74 30                	je     80104ec0 <sys_fstat+0x60>
  if(argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
80104e90:	83 ec 04             	sub    $0x4,%esp
80104e93:	6a 14                	push   $0x14
80104e95:	53                   	push   %ebx
80104e96:	6a 01                	push   $0x1
80104e98:	e8 03 fb ff ff       	call   801049a0 <argptr>
80104e9d:	83 c4 10             	add    $0x10,%esp
80104ea0:	85 c0                	test   %eax,%eax
80104ea2:	78 1c                	js     80104ec0 <sys_fstat+0x60>
  return filestat(f, st);
80104ea4:	83 ec 08             	sub    $0x8,%esp
80104ea7:	ff 75 f4             	push   -0xc(%ebp)
80104eaa:	56                   	push   %esi
80104eab:	e8 40 c1 ff ff       	call   80100ff0 <filestat>
80104eb0:	83 c4 10             	add    $0x10,%esp
}
80104eb3:	8d 65 f8             	lea    -0x8(%ebp),%esp
80104eb6:	5b                   	pop    %ebx
80104eb7:	5e                   	pop    %esi
80104eb8:	5d                   	pop    %ebp
80104eb9:	c3                   	ret
80104eba:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    return -1;
80104ec0:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
80104ec5:	eb ec                	jmp    80104eb3 <sys_fstat+0x53>
80104ec7:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80104ece:	00 
80104ecf:	90                   	nop

80104ed0 <sys_link>:
{
80104ed0:	55                   	push   %ebp
80104ed1:	89 e5                	mov    %esp,%ebp
80104ed3:	57                   	push   %edi
80104ed4:	56                   	push   %esi
  if(argstr(0, &old) < 0 || argstr(1, &new) < 0)
80104ed5:	8d 45 d4             	lea    -0x2c(%ebp),%eax
{
80104ed8:	53                   	push   %ebx
80104ed9:	83 ec 34             	sub    $0x34,%esp
  if(argstr(0, &old) < 0 || argstr(1, &new) < 0)
80104edc:	50                   	push   %eax
80104edd:	6a 00                	push   $0x0
80104edf:	e8 2c fb ff ff       	call   80104a10 <argstr>
80104ee4:	83 c4 10             	add    $0x10,%esp
80104ee7:	85 c0                	test   %eax,%eax
80104ee9:	0f 88 fb 00 00 00    	js     80104fea <sys_link+0x11a>
80104eef:	83 ec 08             	sub    $0x8,%esp
80104ef2:	8d 45 d0             	lea    -0x30(%ebp),%eax
80104ef5:	50                   	push   %eax
80104ef6:	6a 01                	push   $0x1
80104ef8:	e8 13 fb ff ff       	call   80104a10 <argstr>
80104efd:	83 c4 10             	add    $0x10,%esp
80104f00:	85 c0                	test   %eax,%eax
80104f02:	0f 88 e2 00 00 00    	js     80104fea <sys_link+0x11a>
  begin_op();
80104f08:	e8 33 de ff ff       	call   80102d40 <begin_op>
  if((ip = namei(old)) == 0){
80104f0d:	83 ec 0c             	sub    $0xc,%esp
80104f10:	ff 75 d4             	push   -0x2c(%ebp)
80104f13:	e8 68 d1 ff ff       	call   80102080 <namei>
80104f18:	83 c4 10             	add    $0x10,%esp
80104f1b:	89 c3                	mov    %eax,%ebx
80104f1d:	85 c0                	test   %eax,%eax
80104f1f:	0f 84 df 00 00 00    	je     80105004 <sys_link+0x134>
  ilock(ip);
80104f25:	83 ec 0c             	sub    $0xc,%esp
80104f28:	50                   	push   %eax
80104f29:	e8 72 c8 ff ff       	call   801017a0 <ilock>
  if(ip->type == T_DIR){
80104f2e:	83 c4 10             	add    $0x10,%esp
80104f31:	66 83 7b 50 01       	cmpw   $0x1,0x50(%ebx)
80104f36:	0f 84 b5 00 00 00    	je     80104ff1 <sys_link+0x121>
  iupdate(ip);
80104f3c:	83 ec 0c             	sub    $0xc,%esp
  ip->nlink++;
80104f3f:	66 83 43 56 01       	addw   $0x1,0x56(%ebx)
  if((dp = nameiparent(new, name)) == 0)
80104f44:	8d 7d da             	lea    -0x26(%ebp),%edi
  iupdate(ip);
80104f47:	53                   	push   %ebx
80104f48:	e8 a3 c7 ff ff       	call   801016f0 <iupdate>
  iunlock(ip);
80104f4d:	89 1c 24             	mov    %ebx,(%esp)
80104f50:	e8 2b c9 ff ff       	call   80101880 <iunlock>
  if((dp = nameiparent(new, name)) == 0)
80104f55:	58                   	pop    %eax
80104f56:	5a                   	pop    %edx
80104f57:	57                   	push   %edi
80104f58:	ff 75 d0             	push   -0x30(%ebp)
80104f5b:	e8 40 d1 ff ff       	call   801020a0 <nameiparent>
80104f60:	83 c4 10             	add    $0x10,%esp
80104f63:	89 c6                	mov    %eax,%esi
80104f65:	85 c0                	test   %eax,%eax
80104f67:	74 5b                	je     80104fc4 <sys_link+0xf4>
  ilock(dp);
80104f69:	83 ec 0c             	sub    $0xc,%esp
80104f6c:	50                   	push   %eax
80104f6d:	e8 2e c8 ff ff       	call   801017a0 <ilock>
  if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
80104f72:	8b 03                	mov    (%ebx),%eax
80104f74:	83 c4 10             	add    $0x10,%esp
80104f77:	39 06                	cmp    %eax,(%esi)
80104f79:	75 3d                	jne    80104fb8 <sys_link+0xe8>
80104f7b:	83 ec 04             	sub    $0x4,%esp
80104f7e:	ff 73 04             	push   0x4(%ebx)
80104f81:	57                   	push   %edi
80104f82:	56                   	push   %esi
80104f83:	e8 38 d0 ff ff       	call   80101fc0 <dirlink>
80104f88:	83 c4 10             	add    $0x10,%esp
80104f8b:	85 c0                	test   %eax,%eax
80104f8d:	78 29                	js     80104fb8 <sys_link+0xe8>
  iunlockput(dp);
80104f8f:	83 ec 0c             	sub    $0xc,%esp
80104f92:	56                   	push   %esi
80104f93:	e8 98 ca ff ff       	call   80101a30 <iunlockput>
  iput(ip);
80104f98:	89 1c 24             	mov    %ebx,(%esp)
80104f9b:	e8 30 c9 ff ff       	call   801018d0 <iput>
  end_op();
80104fa0:	e8 0b de ff ff       	call   80102db0 <end_op>
  return 0;
80104fa5:	83 c4 10             	add    $0x10,%esp
80104fa8:	31 c0                	xor    %eax,%eax
}
80104faa:	8d 65 f4             	lea    -0xc(%ebp),%esp
80104fad:	5b                   	pop    %ebx
80104fae:	5e                   	pop    %esi
80104faf:	5f                   	pop    %edi
80104fb0:	5d                   	pop    %ebp
80104fb1:	c3                   	ret
80104fb2:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    iunlockput(dp);
80104fb8:	83 ec 0c             	sub    $0xc,%esp
80104fbb:	56                   	push   %esi
80104fbc:	e8 6f ca ff ff       	call   80101a30 <iunlockput>
    goto bad;
80104fc1:	83 c4 10             	add    $0x10,%esp
  ilock(ip);
80104fc4:	83 ec 0c             	sub    $0xc,%esp
80104fc7:	53                   	push   %ebx
80104fc8:	e8 d3 c7 ff ff       	call   801017a0 <ilock>
  ip->nlink--;
80104fcd:	66 83 6b 56 01       	subw   $0x1,0x56(%ebx)
  iupdate(ip);
80104fd2:	89 1c 24             	mov    %ebx,(%esp)
80104fd5:	e8 16 c7 ff ff       	call   801016f0 <iupdate>
  iunlockput(ip);
80104fda:	89 1c 24             	mov    %ebx,(%esp)
80104fdd:	e8 4e ca ff ff       	call   80101a30 <iunlockput>
  end_op();
80104fe2:	e8 c9 dd ff ff       	call   80102db0 <end_op>
  return -1;
80104fe7:	83 c4 10             	add    $0x10,%esp
    return -1;
80104fea:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
80104fef:	eb b9                	jmp    80104faa <sys_link+0xda>
    iunlockput(ip);
80104ff1:	83 ec 0c             	sub    $0xc,%esp
80104ff4:	53                   	push   %ebx
80104ff5:	e8 36 ca ff ff       	call   80101a30 <iunlockput>
    end_op();
80104ffa:	e8 b1 dd ff ff       	call   80102db0 <end_op>
    return -1;
80104fff:	83 c4 10             	add    $0x10,%esp
80105002:	eb e6                	jmp    80104fea <sys_link+0x11a>
    end_op();
80105004:	e8 a7 dd ff ff       	call   80102db0 <end_op>
    return -1;
80105009:	eb df                	jmp    80104fea <sys_link+0x11a>
8010500b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi

80105010 <sys_unlink>:
{
80105010:	55                   	push   %ebp
80105011:	89 e5                	mov    %esp,%ebp
80105013:	57                   	push   %edi
80105014:	56                   	push   %esi
  if(argstr(0, &path) < 0)
80105015:	8d 45 c0             	lea    -0x40(%ebp),%eax
{
80105018:	53                   	push   %ebx
80105019:	83 ec 54             	sub    $0x54,%esp
  if(argstr(0, &path) < 0)
8010501c:	50                   	push   %eax
8010501d:	6a 00                	push   $0x0
8010501f:	e8 ec f9 ff ff       	call   80104a10 <argstr>
80105024:	83 c4 10             	add    $0x10,%esp
80105027:	85 c0                	test   %eax,%eax
80105029:	0f 88 54 01 00 00    	js     80105183 <sys_unlink+0x173>
  begin_op();
8010502f:	e8 0c dd ff ff       	call   80102d40 <begin_op>
  if((dp = nameiparent(path, name)) == 0){
80105034:	8d 5d ca             	lea    -0x36(%ebp),%ebx
80105037:	83 ec 08             	sub    $0x8,%esp
8010503a:	53                   	push   %ebx
8010503b:	ff 75 c0             	push   -0x40(%ebp)
8010503e:	e8 5d d0 ff ff       	call   801020a0 <nameiparent>
80105043:	83 c4 10             	add    $0x10,%esp
80105046:	89 45 b4             	mov    %eax,-0x4c(%ebp)
80105049:	85 c0                	test   %eax,%eax
8010504b:	0f 84 58 01 00 00    	je     801051a9 <sys_unlink+0x199>
  ilock(dp);
80105051:	8b 7d b4             	mov    -0x4c(%ebp),%edi
80105054:	83 ec 0c             	sub    $0xc,%esp
80105057:	57                   	push   %edi
80105058:	e8 43 c7 ff ff       	call   801017a0 <ilock>
  if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
8010505d:	58                   	pop    %eax
8010505e:	5a                   	pop    %edx
8010505f:	68 09 75 10 80       	push   $0x80107509
80105064:	53                   	push   %ebx
80105065:	e8 66 cc ff ff       	call   80101cd0 <namecmp>
8010506a:	83 c4 10             	add    $0x10,%esp
8010506d:	85 c0                	test   %eax,%eax
8010506f:	0f 84 fb 00 00 00    	je     80105170 <sys_unlink+0x160>
80105075:	83 ec 08             	sub    $0x8,%esp
80105078:	68 08 75 10 80       	push   $0x80107508
8010507d:	53                   	push   %ebx
8010507e:	e8 4d cc ff ff       	call   80101cd0 <namecmp>
80105083:	83 c4 10             	add    $0x10,%esp
80105086:	85 c0                	test   %eax,%eax
80105088:	0f 84 e2 00 00 00    	je     80105170 <sys_unlink+0x160>
  if((ip = dirlookup(dp, name, &off)) == 0)
8010508e:	83 ec 04             	sub    $0x4,%esp
80105091:	8d 45 c4             	lea    -0x3c(%ebp),%eax
80105094:	50                   	push   %eax
80105095:	53                   	push   %ebx
80105096:	57                   	push   %edi
80105097:	e8 54 cc ff ff       	call   80101cf0 <dirlookup>
8010509c:	83 c4 10             	add    $0x10,%esp
8010509f:	89 c3                	mov    %eax,%ebx
801050a1:	85 c0                	test   %eax,%eax
801050a3:	0f 84 c7 00 00 00    	je     80105170 <sys_unlink+0x160>
  ilock(ip);
801050a9:	83 ec 0c             	sub    $0xc,%esp
801050ac:	50                   	push   %eax
801050ad:	e8 ee c6 ff ff       	call   801017a0 <ilock>
  if(ip->nlink < 1)
801050b2:	83 c4 10             	add    $0x10,%esp
801050b5:	66 83 7b 56 00       	cmpw   $0x0,0x56(%ebx)
801050ba:	0f 8e 0a 01 00 00    	jle    801051ca <sys_unlink+0x1ba>
  if(ip->type == T_DIR && !isdirempty(ip)){
801050c0:	66 83 7b 50 01       	cmpw   $0x1,0x50(%ebx)
801050c5:	8d 7d d8             	lea    -0x28(%ebp),%edi
801050c8:	74 66                	je     80105130 <sys_unlink+0x120>
  memset(&de, 0, sizeof(de));
801050ca:	83 ec 04             	sub    $0x4,%esp
801050cd:	6a 10                	push   $0x10
801050cf:	6a 00                	push   $0x0
801050d1:	57                   	push   %edi
801050d2:	e8 c9 f5 ff ff       	call   801046a0 <memset>
  if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
801050d7:	6a 10                	push   $0x10
801050d9:	ff 75 c4             	push   -0x3c(%ebp)
801050dc:	57                   	push   %edi
801050dd:	ff 75 b4             	push   -0x4c(%ebp)
801050e0:	e8 cb ca ff ff       	call   80101bb0 <writei>
801050e5:	83 c4 20             	add    $0x20,%esp
801050e8:	83 f8 10             	cmp    $0x10,%eax
801050eb:	0f 85 cc 00 00 00    	jne    801051bd <sys_unlink+0x1ad>
  if(ip->type == T_DIR){
801050f1:	66 83 7b 50 01       	cmpw   $0x1,0x50(%ebx)
801050f6:	0f 84 94 00 00 00    	je     80105190 <sys_unlink+0x180>
  iunlockput(dp);
801050fc:	83 ec 0c             	sub    $0xc,%esp
801050ff:	ff 75 b4             	push   -0x4c(%ebp)
80105102:	e8 29 c9 ff ff       	call   80101a30 <iunlockput>
  ip->nlink--;
80105107:	66 83 6b 56 01       	subw   $0x1,0x56(%ebx)
  iupdate(ip);
8010510c:	89 1c 24             	mov    %ebx,(%esp)
8010510f:	e8 dc c5 ff ff       	call   801016f0 <iupdate>
  iunlockput(ip);
80105114:	89 1c 24             	mov    %ebx,(%esp)
80105117:	e8 14 c9 ff ff       	call   80101a30 <iunlockput>
  end_op();
8010511c:	e8 8f dc ff ff       	call   80102db0 <end_op>
  return 0;
80105121:	83 c4 10             	add    $0x10,%esp
80105124:	31 c0                	xor    %eax,%eax
}
80105126:	8d 65 f4             	lea    -0xc(%ebp),%esp
80105129:	5b                   	pop    %ebx
8010512a:	5e                   	pop    %esi
8010512b:	5f                   	pop    %edi
8010512c:	5d                   	pop    %ebp
8010512d:	c3                   	ret
8010512e:	66 90                	xchg   %ax,%ax
  for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
80105130:	83 7b 58 20          	cmpl   $0x20,0x58(%ebx)
80105134:	76 94                	jbe    801050ca <sys_unlink+0xba>
80105136:	be 20 00 00 00       	mov    $0x20,%esi
8010513b:	eb 0b                	jmp    80105148 <sys_unlink+0x138>
8010513d:	8d 76 00             	lea    0x0(%esi),%esi
80105140:	83 c6 10             	add    $0x10,%esi
80105143:	3b 73 58             	cmp    0x58(%ebx),%esi
80105146:	73 82                	jae    801050ca <sys_unlink+0xba>
    if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
80105148:	6a 10                	push   $0x10
8010514a:	56                   	push   %esi
8010514b:	57                   	push   %edi
8010514c:	53                   	push   %ebx
8010514d:	e8 5e c9 ff ff       	call   80101ab0 <readi>
80105152:	83 c4 10             	add    $0x10,%esp
80105155:	83 f8 10             	cmp    $0x10,%eax
80105158:	75 56                	jne    801051b0 <sys_unlink+0x1a0>
    if(de.inum != 0)
8010515a:	66 83 7d d8 00       	cmpw   $0x0,-0x28(%ebp)
8010515f:	74 df                	je     80105140 <sys_unlink+0x130>
    iunlockput(ip);
80105161:	83 ec 0c             	sub    $0xc,%esp
80105164:	53                   	push   %ebx
80105165:	e8 c6 c8 ff ff       	call   80101a30 <iunlockput>
    goto bad;
8010516a:	83 c4 10             	add    $0x10,%esp
8010516d:	8d 76 00             	lea    0x0(%esi),%esi
  iunlockput(dp);
80105170:	83 ec 0c             	sub    $0xc,%esp
80105173:	ff 75 b4             	push   -0x4c(%ebp)
80105176:	e8 b5 c8 ff ff       	call   80101a30 <iunlockput>
  end_op();
8010517b:	e8 30 dc ff ff       	call   80102db0 <end_op>
  return -1;
80105180:	83 c4 10             	add    $0x10,%esp
    return -1;
80105183:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
80105188:	eb 9c                	jmp    80105126 <sys_unlink+0x116>
8010518a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    dp->nlink--;
80105190:	8b 45 b4             	mov    -0x4c(%ebp),%eax
    iupdate(dp);
80105193:	83 ec 0c             	sub    $0xc,%esp
    dp->nlink--;
80105196:	66 83 68 56 01       	subw   $0x1,0x56(%eax)
    iupdate(dp);
8010519b:	50                   	push   %eax
8010519c:	e8 4f c5 ff ff       	call   801016f0 <iupdate>
801051a1:	83 c4 10             	add    $0x10,%esp
801051a4:	e9 53 ff ff ff       	jmp    801050fc <sys_unlink+0xec>
    end_op();
801051a9:	e8 02 dc ff ff       	call   80102db0 <end_op>
    return -1;
801051ae:	eb d3                	jmp    80105183 <sys_unlink+0x173>
      panic("isdirempty: readi");
801051b0:	83 ec 0c             	sub    $0xc,%esp
801051b3:	68 2d 75 10 80       	push   $0x8010752d
801051b8:	e8 c3 b1 ff ff       	call   80100380 <panic>
    panic("unlink: writei");
801051bd:	83 ec 0c             	sub    $0xc,%esp
801051c0:	68 3f 75 10 80       	push   $0x8010753f
801051c5:	e8 b6 b1 ff ff       	call   80100380 <panic>
    panic("unlink: nlink < 1");
801051ca:	83 ec 0c             	sub    $0xc,%esp
801051cd:	68 1b 75 10 80       	push   $0x8010751b
801051d2:	e8 a9 b1 ff ff       	call   80100380 <panic>
801051d7:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801051de:	00 
801051df:	90                   	nop

801051e0 <sys_open>:

int
sys_open(void)
{
801051e0:	55                   	push   %ebp
801051e1:	89 e5                	mov    %esp,%ebp
801051e3:	57                   	push   %edi
801051e4:	56                   	push   %esi
  char *path;
  int fd, omode;
  struct file *f;
  struct inode *ip;

  if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
801051e5:	8d 45 e0             	lea    -0x20(%ebp),%eax
{
801051e8:	53                   	push   %ebx
801051e9:	83 ec 24             	sub    $0x24,%esp
  if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
801051ec:	50                   	push   %eax
801051ed:	6a 00                	push   $0x0
801051ef:	e8 1c f8 ff ff       	call   80104a10 <argstr>
801051f4:	83 c4 10             	add    $0x10,%esp
801051f7:	85 c0                	test   %eax,%eax
801051f9:	0f 88 8e 00 00 00    	js     8010528d <sys_open+0xad>
801051ff:	83 ec 08             	sub    $0x8,%esp
80105202:	8d 45 e4             	lea    -0x1c(%ebp),%eax
80105205:	50                   	push   %eax
80105206:	6a 01                	push   $0x1
80105208:	e8 43 f7 ff ff       	call   80104950 <argint>
8010520d:	83 c4 10             	add    $0x10,%esp
80105210:	85 c0                	test   %eax,%eax
80105212:	78 79                	js     8010528d <sys_open+0xad>
    return -1;

  begin_op();
80105214:	e8 27 db ff ff       	call   80102d40 <begin_op>

  if(omode & O_CREATE){
80105219:	f6 45 e5 02          	testb  $0x2,-0x1b(%ebp)
8010521d:	75 79                	jne    80105298 <sys_open+0xb8>
    if(ip == 0){
      end_op();
      return -1;
    }
  } else {
    if((ip = namei(path)) == 0){
8010521f:	83 ec 0c             	sub    $0xc,%esp
80105222:	ff 75 e0             	push   -0x20(%ebp)
80105225:	e8 56 ce ff ff       	call   80102080 <namei>
8010522a:	83 c4 10             	add    $0x10,%esp
8010522d:	89 c6                	mov    %eax,%esi
8010522f:	85 c0                	test   %eax,%eax
80105231:	0f 84 7e 00 00 00    	je     801052b5 <sys_open+0xd5>
      end_op();
      return -1;
    }
    ilock(ip);
80105237:	83 ec 0c             	sub    $0xc,%esp
8010523a:	50                   	push   %eax
8010523b:	e8 60 c5 ff ff       	call   801017a0 <ilock>
    if(ip->type == T_DIR && omode != O_RDONLY){
80105240:	83 c4 10             	add    $0x10,%esp
80105243:	66 83 7e 50 01       	cmpw   $0x1,0x50(%esi)
80105248:	0f 84 ba 00 00 00    	je     80105308 <sys_open+0x128>
      end_op();
      return -1;
    }
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
8010524e:	e8 fd bb ff ff       	call   80100e50 <filealloc>
80105253:	89 c7                	mov    %eax,%edi
80105255:	85 c0                	test   %eax,%eax
80105257:	74 23                	je     8010527c <sys_open+0x9c>
  struct proc *curproc = myproc();
80105259:	e8 02 e7 ff ff       	call   80103960 <myproc>
  for(fd = 0; fd < NOFILE; fd++){
8010525e:	31 db                	xor    %ebx,%ebx
    if(curproc->ofile[fd] == 0){
80105260:	8b 54 98 28          	mov    0x28(%eax,%ebx,4),%edx
80105264:	85 d2                	test   %edx,%edx
80105266:	74 58                	je     801052c0 <sys_open+0xe0>
  for(fd = 0; fd < NOFILE; fd++){
80105268:	83 c3 01             	add    $0x1,%ebx
8010526b:	83 fb 10             	cmp    $0x10,%ebx
8010526e:	75 f0                	jne    80105260 <sys_open+0x80>
    if(f)
      fileclose(f);
80105270:	83 ec 0c             	sub    $0xc,%esp
80105273:	57                   	push   %edi
80105274:	e8 97 bc ff ff       	call   80100f10 <fileclose>
80105279:	83 c4 10             	add    $0x10,%esp
    iunlockput(ip);
8010527c:	83 ec 0c             	sub    $0xc,%esp
8010527f:	56                   	push   %esi
80105280:	e8 ab c7 ff ff       	call   80101a30 <iunlockput>
    end_op();
80105285:	e8 26 db ff ff       	call   80102db0 <end_op>
    return -1;
8010528a:	83 c4 10             	add    $0x10,%esp
    return -1;
8010528d:	bb ff ff ff ff       	mov    $0xffffffff,%ebx
80105292:	eb 65                	jmp    801052f9 <sys_open+0x119>
80105294:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    ip = create(path, T_FILE, 0, 0);
80105298:	83 ec 0c             	sub    $0xc,%esp
8010529b:	31 c9                	xor    %ecx,%ecx
8010529d:	ba 02 00 00 00       	mov    $0x2,%edx
801052a2:	6a 00                	push   $0x0
801052a4:	8b 45 e0             	mov    -0x20(%ebp),%eax
801052a7:	e8 54 f8 ff ff       	call   80104b00 <create>
    if(ip == 0){
801052ac:	83 c4 10             	add    $0x10,%esp
    ip = create(path, T_FILE, 0, 0);
801052af:	89 c6                	mov    %eax,%esi
    if(ip == 0){
801052b1:	85 c0                	test   %eax,%eax
801052b3:	75 99                	jne    8010524e <sys_open+0x6e>
      end_op();
801052b5:	e8 f6 da ff ff       	call   80102db0 <end_op>
      return -1;
801052ba:	eb d1                	jmp    8010528d <sys_open+0xad>
801052bc:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
  }
  iunlock(ip);
801052c0:	83 ec 0c             	sub    $0xc,%esp
      curproc->ofile[fd] = f;
801052c3:	89 7c 98 28          	mov    %edi,0x28(%eax,%ebx,4)
  iunlock(ip);
801052c7:	56                   	push   %esi
801052c8:	e8 b3 c5 ff ff       	call   80101880 <iunlock>
  end_op();
801052cd:	e8 de da ff ff       	call   80102db0 <end_op>

  f->type = FD_INODE;
801052d2:	c7 07 02 00 00 00    	movl   $0x2,(%edi)
  f->ip = ip;
  f->off = 0;
  f->readable = !(omode & O_WRONLY);
801052d8:	8b 55 e4             	mov    -0x1c(%ebp),%edx
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
801052db:	83 c4 10             	add    $0x10,%esp
  f->ip = ip;
801052de:	89 77 10             	mov    %esi,0x10(%edi)
  f->readable = !(omode & O_WRONLY);
801052e1:	89 d0                	mov    %edx,%eax
  f->off = 0;
801052e3:	c7 47 14 00 00 00 00 	movl   $0x0,0x14(%edi)
  f->readable = !(omode & O_WRONLY);
801052ea:	f7 d0                	not    %eax
801052ec:	83 e0 01             	and    $0x1,%eax
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
801052ef:	83 e2 03             	and    $0x3,%edx
  f->readable = !(omode & O_WRONLY);
801052f2:	88 47 08             	mov    %al,0x8(%edi)
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
801052f5:	0f 95 47 09          	setne  0x9(%edi)
  return fd;
}
801052f9:	8d 65 f4             	lea    -0xc(%ebp),%esp
801052fc:	89 d8                	mov    %ebx,%eax
801052fe:	5b                   	pop    %ebx
801052ff:	5e                   	pop    %esi
80105300:	5f                   	pop    %edi
80105301:	5d                   	pop    %ebp
80105302:	c3                   	ret
80105303:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
    if(ip->type == T_DIR && omode != O_RDONLY){
80105308:	8b 4d e4             	mov    -0x1c(%ebp),%ecx
8010530b:	85 c9                	test   %ecx,%ecx
8010530d:	0f 84 3b ff ff ff    	je     8010524e <sys_open+0x6e>
80105313:	e9 64 ff ff ff       	jmp    8010527c <sys_open+0x9c>
80105318:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010531f:	00 

80105320 <sys_mkdir>:

int
sys_mkdir(void)
{
80105320:	55                   	push   %ebp
80105321:	89 e5                	mov    %esp,%ebp
80105323:	83 ec 18             	sub    $0x18,%esp
  char *path;
  struct inode *ip;

  begin_op();
80105326:	e8 15 da ff ff       	call   80102d40 <begin_op>
  if(argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0){
8010532b:	83 ec 08             	sub    $0x8,%esp
8010532e:	8d 45 f4             	lea    -0xc(%ebp),%eax
80105331:	50                   	push   %eax
80105332:	6a 00                	push   $0x0
80105334:	e8 d7 f6 ff ff       	call   80104a10 <argstr>
80105339:	83 c4 10             	add    $0x10,%esp
8010533c:	85 c0                	test   %eax,%eax
8010533e:	78 30                	js     80105370 <sys_mkdir+0x50>
80105340:	83 ec 0c             	sub    $0xc,%esp
80105343:	8b 45 f4             	mov    -0xc(%ebp),%eax
80105346:	31 c9                	xor    %ecx,%ecx
80105348:	ba 01 00 00 00       	mov    $0x1,%edx
8010534d:	6a 00                	push   $0x0
8010534f:	e8 ac f7 ff ff       	call   80104b00 <create>
80105354:	83 c4 10             	add    $0x10,%esp
80105357:	85 c0                	test   %eax,%eax
80105359:	74 15                	je     80105370 <sys_mkdir+0x50>
    end_op();
    return -1;
  }
  iunlockput(ip);
8010535b:	83 ec 0c             	sub    $0xc,%esp
8010535e:	50                   	push   %eax
8010535f:	e8 cc c6 ff ff       	call   80101a30 <iunlockput>
  end_op();
80105364:	e8 47 da ff ff       	call   80102db0 <end_op>
  return 0;
80105369:	83 c4 10             	add    $0x10,%esp
8010536c:	31 c0                	xor    %eax,%eax
}
8010536e:	c9                   	leave
8010536f:	c3                   	ret
    end_op();
80105370:	e8 3b da ff ff       	call   80102db0 <end_op>
    return -1;
80105375:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
}
8010537a:	c9                   	leave
8010537b:	c3                   	ret
8010537c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi

80105380 <sys_mknod>:

int
sys_mknod(void)
{
80105380:	55                   	push   %ebp
80105381:	89 e5                	mov    %esp,%ebp
80105383:	83 ec 18             	sub    $0x18,%esp
  struct inode *ip;
  char *path;
  int major, minor;

  begin_op();
80105386:	e8 b5 d9 ff ff       	call   80102d40 <begin_op>
  if((argstr(0, &path)) < 0 ||
8010538b:	83 ec 08             	sub    $0x8,%esp
8010538e:	8d 45 ec             	lea    -0x14(%ebp),%eax
80105391:	50                   	push   %eax
80105392:	6a 00                	push   $0x0
80105394:	e8 77 f6 ff ff       	call   80104a10 <argstr>
80105399:	83 c4 10             	add    $0x10,%esp
8010539c:	85 c0                	test   %eax,%eax
8010539e:	78 60                	js     80105400 <sys_mknod+0x80>
     argint(1, &major) < 0 ||
801053a0:	83 ec 08             	sub    $0x8,%esp
801053a3:	8d 45 f0             	lea    -0x10(%ebp),%eax
801053a6:	50                   	push   %eax
801053a7:	6a 01                	push   $0x1
801053a9:	e8 a2 f5 ff ff       	call   80104950 <argint>
  if((argstr(0, &path)) < 0 ||
801053ae:	83 c4 10             	add    $0x10,%esp
801053b1:	85 c0                	test   %eax,%eax
801053b3:	78 4b                	js     80105400 <sys_mknod+0x80>
     argint(2, &minor) < 0 ||
801053b5:	83 ec 08             	sub    $0x8,%esp
801053b8:	8d 45 f4             	lea    -0xc(%ebp),%eax
801053bb:	50                   	push   %eax
801053bc:	6a 02                	push   $0x2
801053be:	e8 8d f5 ff ff       	call   80104950 <argint>
     argint(1, &major) < 0 ||
801053c3:	83 c4 10             	add    $0x10,%esp
801053c6:	85 c0                	test   %eax,%eax
801053c8:	78 36                	js     80105400 <sys_mknod+0x80>
     (ip = create(path, T_DEV, major, minor)) == 0){
801053ca:	0f bf 45 f4          	movswl -0xc(%ebp),%eax
801053ce:	83 ec 0c             	sub    $0xc,%esp
801053d1:	0f bf 4d f0          	movswl -0x10(%ebp),%ecx
801053d5:	ba 03 00 00 00       	mov    $0x3,%edx
801053da:	50                   	push   %eax
801053db:	8b 45 ec             	mov    -0x14(%ebp),%eax
801053de:	e8 1d f7 ff ff       	call   80104b00 <create>
     argint(2, &minor) < 0 ||
801053e3:	83 c4 10             	add    $0x10,%esp
801053e6:	85 c0                	test   %eax,%eax
801053e8:	74 16                	je     80105400 <sys_mknod+0x80>
    end_op();
    return -1;
  }
  iunlockput(ip);
801053ea:	83 ec 0c             	sub    $0xc,%esp
801053ed:	50                   	push   %eax
801053ee:	e8 3d c6 ff ff       	call   80101a30 <iunlockput>
  end_op();
801053f3:	e8 b8 d9 ff ff       	call   80102db0 <end_op>
  return 0;
801053f8:	83 c4 10             	add    $0x10,%esp
801053fb:	31 c0                	xor    %eax,%eax
}
801053fd:	c9                   	leave
801053fe:	c3                   	ret
801053ff:	90                   	nop
    end_op();
80105400:	e8 ab d9 ff ff       	call   80102db0 <end_op>
    return -1;
80105405:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
}
8010540a:	c9                   	leave
8010540b:	c3                   	ret
8010540c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi

80105410 <sys_chdir>:

int
sys_chdir(void)
{
80105410:	55                   	push   %ebp
80105411:	89 e5                	mov    %esp,%ebp
80105413:	56                   	push   %esi
80105414:	53                   	push   %ebx
80105415:	83 ec 10             	sub    $0x10,%esp
  char *path;
  struct inode *ip;
  struct proc *curproc = myproc();
80105418:	e8 43 e5 ff ff       	call   80103960 <myproc>
8010541d:	89 c6                	mov    %eax,%esi
  
  begin_op();
8010541f:	e8 1c d9 ff ff       	call   80102d40 <begin_op>
  if(argstr(0, &path) < 0 || (ip = namei(path)) == 0){
80105424:	83 ec 08             	sub    $0x8,%esp
80105427:	8d 45 f4             	lea    -0xc(%ebp),%eax
8010542a:	50                   	push   %eax
8010542b:	6a 00                	push   $0x0
8010542d:	e8 de f5 ff ff       	call   80104a10 <argstr>
80105432:	83 c4 10             	add    $0x10,%esp
80105435:	85 c0                	test   %eax,%eax
80105437:	78 77                	js     801054b0 <sys_chdir+0xa0>
80105439:	83 ec 0c             	sub    $0xc,%esp
8010543c:	ff 75 f4             	push   -0xc(%ebp)
8010543f:	e8 3c cc ff ff       	call   80102080 <namei>
80105444:	83 c4 10             	add    $0x10,%esp
80105447:	89 c3                	mov    %eax,%ebx
80105449:	85 c0                	test   %eax,%eax
8010544b:	74 63                	je     801054b0 <sys_chdir+0xa0>
    end_op();
    return -1;
  }
  ilock(ip);
8010544d:	83 ec 0c             	sub    $0xc,%esp
80105450:	50                   	push   %eax
80105451:	e8 4a c3 ff ff       	call   801017a0 <ilock>
  if(ip->type != T_DIR){
80105456:	83 c4 10             	add    $0x10,%esp
80105459:	66 83 7b 50 01       	cmpw   $0x1,0x50(%ebx)
8010545e:	75 30                	jne    80105490 <sys_chdir+0x80>
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
80105460:	83 ec 0c             	sub    $0xc,%esp
80105463:	53                   	push   %ebx
80105464:	e8 17 c4 ff ff       	call   80101880 <iunlock>
  iput(curproc->cwd);
80105469:	58                   	pop    %eax
8010546a:	ff 76 68             	push   0x68(%esi)
8010546d:	e8 5e c4 ff ff       	call   801018d0 <iput>
  end_op();
80105472:	e8 39 d9 ff ff       	call   80102db0 <end_op>
  curproc->cwd = ip;
80105477:	89 5e 68             	mov    %ebx,0x68(%esi)
  return 0;
8010547a:	83 c4 10             	add    $0x10,%esp
8010547d:	31 c0                	xor    %eax,%eax
}
8010547f:	8d 65 f8             	lea    -0x8(%ebp),%esp
80105482:	5b                   	pop    %ebx
80105483:	5e                   	pop    %esi
80105484:	5d                   	pop    %ebp
80105485:	c3                   	ret
80105486:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010548d:	00 
8010548e:	66 90                	xchg   %ax,%ax
    iunlockput(ip);
80105490:	83 ec 0c             	sub    $0xc,%esp
80105493:	53                   	push   %ebx
80105494:	e8 97 c5 ff ff       	call   80101a30 <iunlockput>
    end_op();
80105499:	e8 12 d9 ff ff       	call   80102db0 <end_op>
    return -1;
8010549e:	83 c4 10             	add    $0x10,%esp
    return -1;
801054a1:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
801054a6:	eb d7                	jmp    8010547f <sys_chdir+0x6f>
801054a8:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801054af:	00 
    end_op();
801054b0:	e8 fb d8 ff ff       	call   80102db0 <end_op>
    return -1;
801054b5:	eb ea                	jmp    801054a1 <sys_chdir+0x91>
801054b7:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801054be:	00 
801054bf:	90                   	nop

801054c0 <sys_exec>:

int
sys_exec(void)
{
801054c0:	55                   	push   %ebp
801054c1:	89 e5                	mov    %esp,%ebp
801054c3:	57                   	push   %edi
801054c4:	56                   	push   %esi
  char *path, *argv[MAXARG];
  int i;
  uint uargv, uarg;

  if(argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0){
801054c5:	8d 85 5c ff ff ff    	lea    -0xa4(%ebp),%eax
{
801054cb:	53                   	push   %ebx
801054cc:	81 ec a4 00 00 00    	sub    $0xa4,%esp
  if(argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0){
801054d2:	50                   	push   %eax
801054d3:	6a 00                	push   $0x0
801054d5:	e8 36 f5 ff ff       	call   80104a10 <argstr>
801054da:	83 c4 10             	add    $0x10,%esp
801054dd:	85 c0                	test   %eax,%eax
801054df:	0f 88 87 00 00 00    	js     8010556c <sys_exec+0xac>
801054e5:	83 ec 08             	sub    $0x8,%esp
801054e8:	8d 85 60 ff ff ff    	lea    -0xa0(%ebp),%eax
801054ee:	50                   	push   %eax
801054ef:	6a 01                	push   $0x1
801054f1:	e8 5a f4 ff ff       	call   80104950 <argint>
801054f6:	83 c4 10             	add    $0x10,%esp
801054f9:	85 c0                	test   %eax,%eax
801054fb:	78 6f                	js     8010556c <sys_exec+0xac>
    return -1;
  }
  memset(argv, 0, sizeof(argv));
801054fd:	83 ec 04             	sub    $0x4,%esp
80105500:	8d b5 68 ff ff ff    	lea    -0x98(%ebp),%esi
  for(i=0;; i++){
80105506:	31 db                	xor    %ebx,%ebx
  memset(argv, 0, sizeof(argv));
80105508:	68 80 00 00 00       	push   $0x80
8010550d:	6a 00                	push   $0x0
8010550f:	56                   	push   %esi
80105510:	e8 8b f1 ff ff       	call   801046a0 <memset>
80105515:	83 c4 10             	add    $0x10,%esp
80105518:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010551f:	00 
    if(i >= NELEM(argv))
      return -1;
    if(fetchint(uargv+4*i, (int*)&uarg) < 0)
80105520:	83 ec 08             	sub    $0x8,%esp
80105523:	8d 85 64 ff ff ff    	lea    -0x9c(%ebp),%eax
80105529:	8d 3c 9d 00 00 00 00 	lea    0x0(,%ebx,4),%edi
80105530:	50                   	push   %eax
80105531:	8b 85 60 ff ff ff    	mov    -0xa0(%ebp),%eax
80105537:	01 f8                	add    %edi,%eax
80105539:	50                   	push   %eax
8010553a:	e8 81 f3 ff ff       	call   801048c0 <fetchint>
8010553f:	83 c4 10             	add    $0x10,%esp
80105542:	85 c0                	test   %eax,%eax
80105544:	78 26                	js     8010556c <sys_exec+0xac>
      return -1;
    if(uarg == 0){
80105546:	8b 85 64 ff ff ff    	mov    -0x9c(%ebp),%eax
8010554c:	85 c0                	test   %eax,%eax
8010554e:	74 30                	je     80105580 <sys_exec+0xc0>
      argv[i] = 0;
      break;
    }
    if(fetchstr(uarg, &argv[i]) < 0)
80105550:	83 ec 08             	sub    $0x8,%esp
80105553:	8d 14 3e             	lea    (%esi,%edi,1),%edx
80105556:	52                   	push   %edx
80105557:	50                   	push   %eax
80105558:	e8 a3 f3 ff ff       	call   80104900 <fetchstr>
8010555d:	83 c4 10             	add    $0x10,%esp
80105560:	85 c0                	test   %eax,%eax
80105562:	78 08                	js     8010556c <sys_exec+0xac>
  for(i=0;; i++){
80105564:	83 c3 01             	add    $0x1,%ebx
    if(i >= NELEM(argv))
80105567:	83 fb 20             	cmp    $0x20,%ebx
8010556a:	75 b4                	jne    80105520 <sys_exec+0x60>
      return -1;
  }
  return exec(path, argv);
}
8010556c:	8d 65 f4             	lea    -0xc(%ebp),%esp
    return -1;
8010556f:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
}
80105574:	5b                   	pop    %ebx
80105575:	5e                   	pop    %esi
80105576:	5f                   	pop    %edi
80105577:	5d                   	pop    %ebp
80105578:	c3                   	ret
80105579:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
      argv[i] = 0;
80105580:	c7 84 9d 68 ff ff ff 	movl   $0x0,-0x98(%ebp,%ebx,4)
80105587:	00 00 00 00 
  return exec(path, argv);
8010558b:	83 ec 08             	sub    $0x8,%esp
8010558e:	56                   	push   %esi
8010558f:	ff b5 5c ff ff ff    	push   -0xa4(%ebp)
80105595:	e8 16 b5 ff ff       	call   80100ab0 <exec>
8010559a:	83 c4 10             	add    $0x10,%esp
}
8010559d:	8d 65 f4             	lea    -0xc(%ebp),%esp
801055a0:	5b                   	pop    %ebx
801055a1:	5e                   	pop    %esi
801055a2:	5f                   	pop    %edi
801055a3:	5d                   	pop    %ebp
801055a4:	c3                   	ret
801055a5:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801055ac:	00 
801055ad:	8d 76 00             	lea    0x0(%esi),%esi

801055b0 <sys_pipe>:

int
sys_pipe(void)
{
801055b0:	55                   	push   %ebp
801055b1:	89 e5                	mov    %esp,%ebp
801055b3:	57                   	push   %edi
801055b4:	56                   	push   %esi
  int *fd;
  struct file *rf, *wf;
  int fd0, fd1;

  if(argptr(0, (void*)&fd, 2*sizeof(fd[0])) < 0)
801055b5:	8d 45 dc             	lea    -0x24(%ebp),%eax
{
801055b8:	53                   	push   %ebx
801055b9:	83 ec 20             	sub    $0x20,%esp
  if(argptr(0, (void*)&fd, 2*sizeof(fd[0])) < 0)
801055bc:	6a 08                	push   $0x8
801055be:	50                   	push   %eax
801055bf:	6a 00                	push   $0x0
801055c1:	e8 da f3 ff ff       	call   801049a0 <argptr>
801055c6:	83 c4 10             	add    $0x10,%esp
801055c9:	85 c0                	test   %eax,%eax
801055cb:	0f 88 8b 00 00 00    	js     8010565c <sys_pipe+0xac>
    return -1;
  if(pipealloc(&rf, &wf) < 0)
801055d1:	83 ec 08             	sub    $0x8,%esp
801055d4:	8d 45 e4             	lea    -0x1c(%ebp),%eax
801055d7:	50                   	push   %eax
801055d8:	8d 45 e0             	lea    -0x20(%ebp),%eax
801055db:	50                   	push   %eax
801055dc:	e8 2f de ff ff       	call   80103410 <pipealloc>
801055e1:	83 c4 10             	add    $0x10,%esp
801055e4:	85 c0                	test   %eax,%eax
801055e6:	78 74                	js     8010565c <sys_pipe+0xac>
    return -1;
  fd0 = -1;
  if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
801055e8:	8b 7d e0             	mov    -0x20(%ebp),%edi
  for(fd = 0; fd < NOFILE; fd++){
801055eb:	31 db                	xor    %ebx,%ebx
  struct proc *curproc = myproc();
801055ed:	e8 6e e3 ff ff       	call   80103960 <myproc>
    if(curproc->ofile[fd] == 0){
801055f2:	8b 74 98 28          	mov    0x28(%eax,%ebx,4),%esi
801055f6:	85 f6                	test   %esi,%esi
801055f8:	74 16                	je     80105610 <sys_pipe+0x60>
801055fa:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
  for(fd = 0; fd < NOFILE; fd++){
80105600:	83 c3 01             	add    $0x1,%ebx
80105603:	83 fb 10             	cmp    $0x10,%ebx
80105606:	74 3d                	je     80105645 <sys_pipe+0x95>
    if(curproc->ofile[fd] == 0){
80105608:	8b 74 98 28          	mov    0x28(%eax,%ebx,4),%esi
8010560c:	85 f6                	test   %esi,%esi
8010560e:	75 f0                	jne    80105600 <sys_pipe+0x50>
      curproc->ofile[fd] = f;
80105610:	8d 73 08             	lea    0x8(%ebx),%esi
80105613:	89 7c b0 08          	mov    %edi,0x8(%eax,%esi,4)
  if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
80105617:	8b 7d e4             	mov    -0x1c(%ebp),%edi
  struct proc *curproc = myproc();
8010561a:	e8 41 e3 ff ff       	call   80103960 <myproc>
  for(fd = 0; fd < NOFILE; fd++){
8010561f:	31 d2                	xor    %edx,%edx
80105621:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    if(curproc->ofile[fd] == 0){
80105628:	8b 4c 90 28          	mov    0x28(%eax,%edx,4),%ecx
8010562c:	85 c9                	test   %ecx,%ecx
8010562e:	74 38                	je     80105668 <sys_pipe+0xb8>
  for(fd = 0; fd < NOFILE; fd++){
80105630:	83 c2 01             	add    $0x1,%edx
80105633:	83 fa 10             	cmp    $0x10,%edx
80105636:	75 f0                	jne    80105628 <sys_pipe+0x78>
    if(fd0 >= 0)
      myproc()->ofile[fd0] = 0;
80105638:	e8 23 e3 ff ff       	call   80103960 <myproc>
8010563d:	c7 44 b0 08 00 00 00 	movl   $0x0,0x8(%eax,%esi,4)
80105644:	00 
    fileclose(rf);
80105645:	83 ec 0c             	sub    $0xc,%esp
80105648:	ff 75 e0             	push   -0x20(%ebp)
8010564b:	e8 c0 b8 ff ff       	call   80100f10 <fileclose>
    fileclose(wf);
80105650:	58                   	pop    %eax
80105651:	ff 75 e4             	push   -0x1c(%ebp)
80105654:	e8 b7 b8 ff ff       	call   80100f10 <fileclose>
    return -1;
80105659:	83 c4 10             	add    $0x10,%esp
    return -1;
8010565c:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
80105661:	eb 16                	jmp    80105679 <sys_pipe+0xc9>
80105663:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
      curproc->ofile[fd] = f;
80105668:	89 7c 90 28          	mov    %edi,0x28(%eax,%edx,4)
  }
  fd[0] = fd0;
8010566c:	8b 45 dc             	mov    -0x24(%ebp),%eax
8010566f:	89 18                	mov    %ebx,(%eax)
  fd[1] = fd1;
80105671:	8b 45 dc             	mov    -0x24(%ebp),%eax
80105674:	89 50 04             	mov    %edx,0x4(%eax)
  return 0;
80105677:	31 c0                	xor    %eax,%eax
}
80105679:	8d 65 f4             	lea    -0xc(%ebp),%esp
8010567c:	5b                   	pop    %ebx
8010567d:	5e                   	pop    %esi
8010567e:	5f                   	pop    %edi
8010567f:	5d                   	pop    %ebp
80105680:	c3                   	ret
80105681:	66 90                	xchg   %ax,%ax
80105683:	66 90                	xchg   %ax,%ax
80105685:	66 90                	xchg   %ax,%ax
80105687:	66 90                	xchg   %ax,%ax
80105689:	66 90                	xchg   %ax,%ax
8010568b:	66 90                	xchg   %ax,%ax
8010568d:	66 90                	xchg   %ax,%ax
8010568f:	90                   	nop

80105690 <sys_fork>:
#include "proc.h"

int
sys_fork(void)
{
  return fork();
80105690:	e9 6b e4 ff ff       	jmp    80103b00 <fork>
80105695:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010569c:	00 
8010569d:	8d 76 00             	lea    0x0(%esi),%esi

801056a0 <sys_exit>:
}

int
sys_exit(void)
{
801056a0:	55                   	push   %ebp
801056a1:	89 e5                	mov    %esp,%ebp
801056a3:	83 ec 08             	sub    $0x8,%esp
  exit();
801056a6:	e8 c5 e6 ff ff       	call   80103d70 <exit>
  return 0;  // not reached
}
801056ab:	31 c0                	xor    %eax,%eax
801056ad:	c9                   	leave
801056ae:	c3                   	ret
801056af:	90                   	nop

801056b0 <sys_wait>:

int
sys_wait(void)
{
  return wait();
801056b0:	e9 eb e7 ff ff       	jmp    80103ea0 <wait>
801056b5:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801056bc:	00 
801056bd:	8d 76 00             	lea    0x0(%esi),%esi

801056c0 <sys_kill>:
}

int
sys_kill(void)
{
801056c0:	55                   	push   %ebp
801056c1:	89 e5                	mov    %esp,%ebp
801056c3:	83 ec 20             	sub    $0x20,%esp
  int pid;

  if(argint(0, &pid) < 0)
801056c6:	8d 45 f4             	lea    -0xc(%ebp),%eax
801056c9:	50                   	push   %eax
801056ca:	6a 00                	push   $0x0
801056cc:	e8 7f f2 ff ff       	call   80104950 <argint>
801056d1:	83 c4 10             	add    $0x10,%esp
801056d4:	85 c0                	test   %eax,%eax
801056d6:	78 18                	js     801056f0 <sys_kill+0x30>
    return -1;
  return kill(pid);
801056d8:	83 ec 0c             	sub    $0xc,%esp
801056db:	ff 75 f4             	push   -0xc(%ebp)
801056de:	e8 5d ea ff ff       	call   80104140 <kill>
801056e3:	83 c4 10             	add    $0x10,%esp
}
801056e6:	c9                   	leave
801056e7:	c3                   	ret
801056e8:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801056ef:	00 
801056f0:	c9                   	leave
    return -1;
801056f1:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
}
801056f6:	c3                   	ret
801056f7:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801056fe:	00 
801056ff:	90                   	nop

80105700 <sys_getpid>:

int
sys_getpid(void)
{
80105700:	55                   	push   %ebp
80105701:	89 e5                	mov    %esp,%ebp
80105703:	83 ec 08             	sub    $0x8,%esp
  return myproc()->pid;
80105706:	e8 55 e2 ff ff       	call   80103960 <myproc>
8010570b:	8b 40 10             	mov    0x10(%eax),%eax
}
8010570e:	c9                   	leave
8010570f:	c3                   	ret

80105710 <sys_sbrk>:

int
sys_sbrk(void)
{
80105710:	55                   	push   %ebp
80105711:	89 e5                	mov    %esp,%ebp
80105713:	53                   	push   %ebx
  int addr;
  int n;

  if(argint(0, &n) < 0)
80105714:	8d 45 f4             	lea    -0xc(%ebp),%eax
{
80105717:	83 ec 1c             	sub    $0x1c,%esp
  if(argint(0, &n) < 0)
8010571a:	50                   	push   %eax
8010571b:	6a 00                	push   $0x0
8010571d:	e8 2e f2 ff ff       	call   80104950 <argint>
80105722:	83 c4 10             	add    $0x10,%esp
80105725:	85 c0                	test   %eax,%eax
80105727:	78 27                	js     80105750 <sys_sbrk+0x40>
    return -1;
  addr = myproc()->sz;
80105729:	e8 32 e2 ff ff       	call   80103960 <myproc>
  if(growproc(n) < 0)
8010572e:	83 ec 0c             	sub    $0xc,%esp
  addr = myproc()->sz;
80105731:	8b 18                	mov    (%eax),%ebx
  if(growproc(n) < 0)
80105733:	ff 75 f4             	push   -0xc(%ebp)
80105736:	e8 45 e3 ff ff       	call   80103a80 <growproc>
8010573b:	83 c4 10             	add    $0x10,%esp
8010573e:	85 c0                	test   %eax,%eax
80105740:	78 0e                	js     80105750 <sys_sbrk+0x40>
    return -1;
  return addr;
}
80105742:	89 d8                	mov    %ebx,%eax
80105744:	8b 5d fc             	mov    -0x4(%ebp),%ebx
80105747:	c9                   	leave
80105748:	c3                   	ret
80105749:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    return -1;
80105750:	bb ff ff ff ff       	mov    $0xffffffff,%ebx
80105755:	eb eb                	jmp    80105742 <sys_sbrk+0x32>
80105757:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010575e:	00 
8010575f:	90                   	nop

80105760 <sys_sleep>:

int
sys_sleep(void)
{
80105760:	55                   	push   %ebp
80105761:	89 e5                	mov    %esp,%ebp
80105763:	53                   	push   %ebx
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
80105764:	8d 45 f4             	lea    -0xc(%ebp),%eax
{
80105767:	83 ec 1c             	sub    $0x1c,%esp
  if(argint(0, &n) < 0)
8010576a:	50                   	push   %eax
8010576b:	6a 00                	push   $0x0
8010576d:	e8 de f1 ff ff       	call   80104950 <argint>
80105772:	83 c4 10             	add    $0x10,%esp
80105775:	85 c0                	test   %eax,%eax
80105777:	78 64                	js     801057dd <sys_sleep+0x7d>
    return -1;
  acquire(&tickslock);
80105779:	83 ec 0c             	sub    $0xc,%esp
8010577c:	68 80 3c 11 80       	push   $0x80113c80
80105781:	e8 1a ee ff ff       	call   801045a0 <acquire>
  ticks0 = ticks;
  while(ticks - ticks0 < n){
80105786:	8b 55 f4             	mov    -0xc(%ebp),%edx
  ticks0 = ticks;
80105789:	8b 1d 60 3c 11 80    	mov    0x80113c60,%ebx
  while(ticks - ticks0 < n){
8010578f:	83 c4 10             	add    $0x10,%esp
80105792:	85 d2                	test   %edx,%edx
80105794:	75 2b                	jne    801057c1 <sys_sleep+0x61>
80105796:	eb 58                	jmp    801057f0 <sys_sleep+0x90>
80105798:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010579f:	00 
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
801057a0:	83 ec 08             	sub    $0x8,%esp
801057a3:	68 80 3c 11 80       	push   $0x80113c80
801057a8:	68 60 3c 11 80       	push   $0x80113c60
801057ad:	e8 6e e8 ff ff       	call   80104020 <sleep>
  while(ticks - ticks0 < n){
801057b2:	a1 60 3c 11 80       	mov    0x80113c60,%eax
801057b7:	83 c4 10             	add    $0x10,%esp
801057ba:	29 d8                	sub    %ebx,%eax
801057bc:	3b 45 f4             	cmp    -0xc(%ebp),%eax
801057bf:	73 2f                	jae    801057f0 <sys_sleep+0x90>
    if(myproc()->killed){
801057c1:	e8 9a e1 ff ff       	call   80103960 <myproc>
801057c6:	8b 40 24             	mov    0x24(%eax),%eax
801057c9:	85 c0                	test   %eax,%eax
801057cb:	74 d3                	je     801057a0 <sys_sleep+0x40>
      release(&tickslock);
801057cd:	83 ec 0c             	sub    $0xc,%esp
801057d0:	68 80 3c 11 80       	push   $0x80113c80
801057d5:	e8 66 ed ff ff       	call   80104540 <release>
      return -1;
801057da:	83 c4 10             	add    $0x10,%esp
  }
  release(&tickslock);
  return 0;
}
801057dd:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    return -1;
801057e0:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
}
801057e5:	c9                   	leave
801057e6:	c3                   	ret
801057e7:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801057ee:	00 
801057ef:	90                   	nop
  release(&tickslock);
801057f0:	83 ec 0c             	sub    $0xc,%esp
801057f3:	68 80 3c 11 80       	push   $0x80113c80
801057f8:	e8 43 ed ff ff       	call   80104540 <release>
}
801057fd:	8b 5d fc             	mov    -0x4(%ebp),%ebx
  return 0;
80105800:	83 c4 10             	add    $0x10,%esp
80105803:	31 c0                	xor    %eax,%eax
}
80105805:	c9                   	leave
80105806:	c3                   	ret
80105807:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010580e:	00 
8010580f:	90                   	nop

80105810 <sys_uptime>:

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
80105810:	55                   	push   %ebp
80105811:	89 e5                	mov    %esp,%ebp
80105813:	53                   	push   %ebx
80105814:	83 ec 10             	sub    $0x10,%esp
  uint xticks;

  acquire(&tickslock);
80105817:	68 80 3c 11 80       	push   $0x80113c80
8010581c:	e8 7f ed ff ff       	call   801045a0 <acquire>
  xticks = ticks;
80105821:	8b 1d 60 3c 11 80    	mov    0x80113c60,%ebx
  release(&tickslock);
80105827:	c7 04 24 80 3c 11 80 	movl   $0x80113c80,(%esp)
8010582e:	e8 0d ed ff ff       	call   80104540 <release>
  return xticks;
}
80105833:	89 d8                	mov    %ebx,%eax
80105835:	8b 5d fc             	mov    -0x4(%ebp),%ebx
80105838:	c9                   	leave
80105839:	c3                   	ret

8010583a <alltraps>:

  # vectors.S sends all traps here.
.globl alltraps
alltraps:
  # Build trap frame.
  pushl %ds
8010583a:	1e                   	push   %ds
  pushl %es
8010583b:	06                   	push   %es
  pushl %fs
8010583c:	0f a0                	push   %fs
  pushl %gs
8010583e:	0f a8                	push   %gs
  pushal
80105840:	60                   	pusha
  
  # Set up data segments.
  movw $(SEG_KDATA<<3), %ax
80105841:	66 b8 10 00          	mov    $0x10,%ax
  movw %ax, %ds
80105845:	8e d8                	mov    %eax,%ds
  movw %ax, %es
80105847:	8e c0                	mov    %eax,%es

  # Call trap(tf), where tf=%esp
  pushl %esp
80105849:	54                   	push   %esp
  call trap
8010584a:	e8 c1 00 00 00       	call   80105910 <trap>
  addl $4, %esp
8010584f:	83 c4 04             	add    $0x4,%esp

80105852 <trapret>:

  # Return falls through to trapret...
.globl trapret
trapret:
  popal
80105852:	61                   	popa
  popl %gs
80105853:	0f a9                	pop    %gs
  popl %fs
80105855:	0f a1                	pop    %fs
  popl %es
80105857:	07                   	pop    %es
  popl %ds
80105858:	1f                   	pop    %ds
  addl $0x8, %esp  # trapno and errcode
80105859:	83 c4 08             	add    $0x8,%esp
  iret
8010585c:	cf                   	iret
8010585d:	66 90                	xchg   %ax,%ax
8010585f:	90                   	nop

80105860 <tvinit>:
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
80105860:	55                   	push   %ebp
  int i;

  for(i = 0; i < 256; i++)
80105861:	31 c0                	xor    %eax,%eax
{
80105863:	89 e5                	mov    %esp,%ebp
80105865:	83 ec 08             	sub    $0x8,%esp
80105868:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010586f:	00 
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
80105870:	8b 14 85 08 a0 10 80 	mov    -0x7fef5ff8(,%eax,4),%edx
80105877:	c7 04 c5 c2 3c 11 80 	movl   $0x8e000008,-0x7feec33e(,%eax,8)
8010587e:	08 00 00 8e 
80105882:	66 89 14 c5 c0 3c 11 	mov    %dx,-0x7feec340(,%eax,8)
80105889:	80 
8010588a:	c1 ea 10             	shr    $0x10,%edx
8010588d:	66 89 14 c5 c6 3c 11 	mov    %dx,-0x7feec33a(,%eax,8)
80105894:	80 
  for(i = 0; i < 256; i++)
80105895:	83 c0 01             	add    $0x1,%eax
80105898:	3d 00 01 00 00       	cmp    $0x100,%eax
8010589d:	75 d1                	jne    80105870 <tvinit+0x10>
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
8010589f:	83 ec 08             	sub    $0x8,%esp
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);
801058a2:	a1 08 a1 10 80       	mov    0x8010a108,%eax
801058a7:	c7 05 c2 3e 11 80 08 	movl   $0xef000008,0x80113ec2
801058ae:	00 00 ef 
  initlock(&tickslock, "time");
801058b1:	68 4e 75 10 80       	push   $0x8010754e
801058b6:	68 80 3c 11 80       	push   $0x80113c80
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);
801058bb:	66 a3 c0 3e 11 80    	mov    %ax,0x80113ec0
801058c1:	c1 e8 10             	shr    $0x10,%eax
801058c4:	66 a3 c6 3e 11 80    	mov    %ax,0x80113ec6
  initlock(&tickslock, "time");
801058ca:	e8 e1 ea ff ff       	call   801043b0 <initlock>
}
801058cf:	83 c4 10             	add    $0x10,%esp
801058d2:	c9                   	leave
801058d3:	c3                   	ret
801058d4:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801058db:	00 
801058dc:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi

801058e0 <idtinit>:

void
idtinit(void)
{
801058e0:	55                   	push   %ebp
  pd[0] = size-1;
801058e1:	b8 ff 07 00 00       	mov    $0x7ff,%eax
801058e6:	89 e5                	mov    %esp,%ebp
801058e8:	83 ec 10             	sub    $0x10,%esp
801058eb:	66 89 45 fa          	mov    %ax,-0x6(%ebp)
  pd[1] = (uint)p;
801058ef:	b8 c0 3c 11 80       	mov    $0x80113cc0,%eax
801058f4:	66 89 45 fc          	mov    %ax,-0x4(%ebp)
  pd[2] = (uint)p >> 16;
801058f8:	c1 e8 10             	shr    $0x10,%eax
801058fb:	66 89 45 fe          	mov    %ax,-0x2(%ebp)
  asm volatile("lidt (%0)" : : "r" (pd));
801058ff:	8d 45 fa             	lea    -0x6(%ebp),%eax
80105902:	0f 01 18             	lidtl  (%eax)
  lidt(idt, sizeof(idt));
}
80105905:	c9                   	leave
80105906:	c3                   	ret
80105907:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010590e:	00 
8010590f:	90                   	nop

80105910 <trap>:

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
80105910:	55                   	push   %ebp
80105911:	89 e5                	mov    %esp,%ebp
80105913:	57                   	push   %edi
80105914:	56                   	push   %esi
80105915:	53                   	push   %ebx
80105916:	83 ec 1c             	sub    $0x1c,%esp
80105919:	8b 5d 08             	mov    0x8(%ebp),%ebx
  if(tf->trapno == T_SYSCALL){
8010591c:	8b 43 30             	mov    0x30(%ebx),%eax
8010591f:	83 f8 40             	cmp    $0x40,%eax
80105922:	0f 84 58 01 00 00    	je     80105a80 <trap+0x170>
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
80105928:	83 e8 20             	sub    $0x20,%eax
8010592b:	83 f8 1f             	cmp    $0x1f,%eax
8010592e:	0f 87 7c 00 00 00    	ja     801059b0 <trap+0xa0>
80105934:	ff 24 85 b8 7a 10 80 	jmp    *-0x7fef8548(,%eax,4)
8010593b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
80105940:	e8 eb c8 ff ff       	call   80102230 <ideintr>
    lapiceoi();
80105945:	e8 a6 cf ff ff       	call   801028f0 <lapiceoi>
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
8010594a:	e8 11 e0 ff ff       	call   80103960 <myproc>
8010594f:	85 c0                	test   %eax,%eax
80105951:	74 1a                	je     8010596d <trap+0x5d>
80105953:	e8 08 e0 ff ff       	call   80103960 <myproc>
80105958:	8b 50 24             	mov    0x24(%eax),%edx
8010595b:	85 d2                	test   %edx,%edx
8010595d:	74 0e                	je     8010596d <trap+0x5d>
8010595f:	0f b7 43 3c          	movzwl 0x3c(%ebx),%eax
80105963:	f7 d0                	not    %eax
80105965:	a8 03                	test   $0x3,%al
80105967:	0f 84 db 01 00 00    	je     80105b48 <trap+0x238>
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
8010596d:	e8 ee df ff ff       	call   80103960 <myproc>
80105972:	85 c0                	test   %eax,%eax
80105974:	74 0f                	je     80105985 <trap+0x75>
80105976:	e8 e5 df ff ff       	call   80103960 <myproc>
8010597b:	83 78 0c 04          	cmpl   $0x4,0xc(%eax)
8010597f:	0f 84 ab 00 00 00    	je     80105a30 <trap+0x120>
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
80105985:	e8 d6 df ff ff       	call   80103960 <myproc>
8010598a:	85 c0                	test   %eax,%eax
8010598c:	74 1a                	je     801059a8 <trap+0x98>
8010598e:	e8 cd df ff ff       	call   80103960 <myproc>
80105993:	8b 40 24             	mov    0x24(%eax),%eax
80105996:	85 c0                	test   %eax,%eax
80105998:	74 0e                	je     801059a8 <trap+0x98>
8010599a:	0f b7 43 3c          	movzwl 0x3c(%ebx),%eax
8010599e:	f7 d0                	not    %eax
801059a0:	a8 03                	test   $0x3,%al
801059a2:	0f 84 05 01 00 00    	je     80105aad <trap+0x19d>
    exit();
}
801059a8:	8d 65 f4             	lea    -0xc(%ebp),%esp
801059ab:	5b                   	pop    %ebx
801059ac:	5e                   	pop    %esi
801059ad:	5f                   	pop    %edi
801059ae:	5d                   	pop    %ebp
801059af:	c3                   	ret
    if(myproc() == 0 || (tf->cs&3) == 0){
801059b0:	e8 ab df ff ff       	call   80103960 <myproc>
801059b5:	8b 7b 38             	mov    0x38(%ebx),%edi
801059b8:	85 c0                	test   %eax,%eax
801059ba:	0f 84 a2 01 00 00    	je     80105b62 <trap+0x252>
801059c0:	f6 43 3c 03          	testb  $0x3,0x3c(%ebx)
801059c4:	0f 84 98 01 00 00    	je     80105b62 <trap+0x252>

static inline uint
rcr2(void)
{
  uint val;
  asm volatile("movl %%cr2,%0" : "=r" (val));
801059ca:	0f 20 d1             	mov    %cr2,%ecx
801059cd:	89 4d d8             	mov    %ecx,-0x28(%ebp)
    cprintf("pid %d %s: trap %d err %d on cpu %d "
801059d0:	e8 6b df ff ff       	call   80103940 <cpuid>
801059d5:	8b 73 30             	mov    0x30(%ebx),%esi
801059d8:	89 45 dc             	mov    %eax,-0x24(%ebp)
801059db:	8b 43 34             	mov    0x34(%ebx),%eax
801059de:	89 45 e4             	mov    %eax,-0x1c(%ebp)
            myproc()->pid, myproc()->name, tf->trapno,
801059e1:	e8 7a df ff ff       	call   80103960 <myproc>
801059e6:	89 45 e0             	mov    %eax,-0x20(%ebp)
801059e9:	e8 72 df ff ff       	call   80103960 <myproc>
    cprintf("pid %d %s: trap %d err %d on cpu %d "
801059ee:	8b 4d d8             	mov    -0x28(%ebp),%ecx
801059f1:	51                   	push   %ecx
801059f2:	57                   	push   %edi
801059f3:	8b 55 dc             	mov    -0x24(%ebp),%edx
801059f6:	52                   	push   %edx
801059f7:	ff 75 e4             	push   -0x1c(%ebp)
801059fa:	56                   	push   %esi
            myproc()->pid, myproc()->name, tf->trapno,
801059fb:	8b 75 e0             	mov    -0x20(%ebp),%esi
801059fe:	83 c6 6c             	add    $0x6c,%esi
    cprintf("pid %d %s: trap %d err %d on cpu %d "
80105a01:	56                   	push   %esi
80105a02:	ff 70 10             	push   0x10(%eax)
80105a05:	68 9c 77 10 80       	push   $0x8010779c
80105a0a:	e8 a1 ac ff ff       	call   801006b0 <cprintf>
    myproc()->killed = 1;
80105a0f:	83 c4 20             	add    $0x20,%esp
80105a12:	e8 49 df ff ff       	call   80103960 <myproc>
80105a17:	c7 40 24 01 00 00 00 	movl   $0x1,0x24(%eax)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
80105a1e:	e8 3d df ff ff       	call   80103960 <myproc>
80105a23:	85 c0                	test   %eax,%eax
80105a25:	0f 85 28 ff ff ff    	jne    80105953 <trap+0x43>
80105a2b:	e9 3d ff ff ff       	jmp    8010596d <trap+0x5d>
  if(myproc() && myproc()->state == RUNNING &&
80105a30:	83 7b 30 20          	cmpl   $0x20,0x30(%ebx)
80105a34:	0f 85 4b ff ff ff    	jne    80105985 <trap+0x75>
    yield();
80105a3a:	e8 91 e5 ff ff       	call   80103fd0 <yield>
80105a3f:	e9 41 ff ff ff       	jmp    80105985 <trap+0x75>
80105a44:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
80105a48:	8b 7b 38             	mov    0x38(%ebx),%edi
80105a4b:	0f b7 73 3c          	movzwl 0x3c(%ebx),%esi
80105a4f:	e8 ec de ff ff       	call   80103940 <cpuid>
80105a54:	57                   	push   %edi
80105a55:	56                   	push   %esi
80105a56:	50                   	push   %eax
80105a57:	68 44 77 10 80       	push   $0x80107744
80105a5c:	e8 4f ac ff ff       	call   801006b0 <cprintf>
    lapiceoi();
80105a61:	e8 8a ce ff ff       	call   801028f0 <lapiceoi>
    break;
80105a66:	83 c4 10             	add    $0x10,%esp
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
80105a69:	e8 f2 de ff ff       	call   80103960 <myproc>
80105a6e:	85 c0                	test   %eax,%eax
80105a70:	0f 85 dd fe ff ff    	jne    80105953 <trap+0x43>
80105a76:	e9 f2 fe ff ff       	jmp    8010596d <trap+0x5d>
80105a7b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
    if(myproc()->killed)
80105a80:	e8 db de ff ff       	call   80103960 <myproc>
80105a85:	8b 70 24             	mov    0x24(%eax),%esi
80105a88:	85 f6                	test   %esi,%esi
80105a8a:	0f 85 c8 00 00 00    	jne    80105b58 <trap+0x248>
    myproc()->tf = tf;
80105a90:	e8 cb de ff ff       	call   80103960 <myproc>
80105a95:	89 58 18             	mov    %ebx,0x18(%eax)
    syscall();
80105a98:	e8 f3 ef ff ff       	call   80104a90 <syscall>
    if(myproc()->killed)
80105a9d:	e8 be de ff ff       	call   80103960 <myproc>
80105aa2:	8b 48 24             	mov    0x24(%eax),%ecx
80105aa5:	85 c9                	test   %ecx,%ecx
80105aa7:	0f 84 fb fe ff ff    	je     801059a8 <trap+0x98>
}
80105aad:	8d 65 f4             	lea    -0xc(%ebp),%esp
80105ab0:	5b                   	pop    %ebx
80105ab1:	5e                   	pop    %esi
80105ab2:	5f                   	pop    %edi
80105ab3:	5d                   	pop    %ebp
      exit();
80105ab4:	e9 b7 e2 ff ff       	jmp    80103d70 <exit>
80105ab9:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    uartintr();
80105ac0:	e8 4b 02 00 00       	call   80105d10 <uartintr>
    lapiceoi();
80105ac5:	e8 26 ce ff ff       	call   801028f0 <lapiceoi>
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
80105aca:	e8 91 de ff ff       	call   80103960 <myproc>
80105acf:	85 c0                	test   %eax,%eax
80105ad1:	0f 85 7c fe ff ff    	jne    80105953 <trap+0x43>
80105ad7:	e9 91 fe ff ff       	jmp    8010596d <trap+0x5d>
80105adc:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    kbdintr();
80105ae0:	e8 db cc ff ff       	call   801027c0 <kbdintr>
    lapiceoi();
80105ae5:	e8 06 ce ff ff       	call   801028f0 <lapiceoi>
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
80105aea:	e8 71 de ff ff       	call   80103960 <myproc>
80105aef:	85 c0                	test   %eax,%eax
80105af1:	0f 85 5c fe ff ff    	jne    80105953 <trap+0x43>
80105af7:	e9 71 fe ff ff       	jmp    8010596d <trap+0x5d>
80105afc:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    if(cpuid() == 0){
80105b00:	e8 3b de ff ff       	call   80103940 <cpuid>
80105b05:	85 c0                	test   %eax,%eax
80105b07:	0f 85 38 fe ff ff    	jne    80105945 <trap+0x35>
      acquire(&tickslock);
80105b0d:	83 ec 0c             	sub    $0xc,%esp
80105b10:	68 80 3c 11 80       	push   $0x80113c80
80105b15:	e8 86 ea ff ff       	call   801045a0 <acquire>
      ticks++;
80105b1a:	83 05 60 3c 11 80 01 	addl   $0x1,0x80113c60
      wakeup(&ticks);
80105b21:	c7 04 24 60 3c 11 80 	movl   $0x80113c60,(%esp)
80105b28:	e8 b3 e5 ff ff       	call   801040e0 <wakeup>
      release(&tickslock);
80105b2d:	c7 04 24 80 3c 11 80 	movl   $0x80113c80,(%esp)
80105b34:	e8 07 ea ff ff       	call   80104540 <release>
80105b39:	83 c4 10             	add    $0x10,%esp
    lapiceoi();
80105b3c:	e9 04 fe ff ff       	jmp    80105945 <trap+0x35>
80105b41:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    exit();
80105b48:	e8 23 e2 ff ff       	call   80103d70 <exit>
80105b4d:	e9 1b fe ff ff       	jmp    8010596d <trap+0x5d>
80105b52:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
      exit();
80105b58:	e8 13 e2 ff ff       	call   80103d70 <exit>
80105b5d:	e9 2e ff ff ff       	jmp    80105a90 <trap+0x180>
80105b62:	0f 20 d6             	mov    %cr2,%esi
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
80105b65:	e8 d6 dd ff ff       	call   80103940 <cpuid>
80105b6a:	83 ec 0c             	sub    $0xc,%esp
80105b6d:	56                   	push   %esi
80105b6e:	57                   	push   %edi
80105b6f:	50                   	push   %eax
80105b70:	ff 73 30             	push   0x30(%ebx)
80105b73:	68 68 77 10 80       	push   $0x80107768
80105b78:	e8 33 ab ff ff       	call   801006b0 <cprintf>
      panic("trap");
80105b7d:	83 c4 14             	add    $0x14,%esp
80105b80:	68 53 75 10 80       	push   $0x80107553
80105b85:	e8 f6 a7 ff ff       	call   80100380 <panic>
80105b8a:	66 90                	xchg   %ax,%ax
80105b8c:	66 90                	xchg   %ax,%ax
80105b8e:	66 90                	xchg   %ax,%ax

80105b90 <uartgetc>:
}

static int
uartgetc(void)
{
  if(!uart)
80105b90:	a1 c0 44 11 80       	mov    0x801144c0,%eax
80105b95:	85 c0                	test   %eax,%eax
80105b97:	74 17                	je     80105bb0 <uartgetc+0x20>
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
80105b99:	ba fd 03 00 00       	mov    $0x3fd,%edx
80105b9e:	ec                   	in     (%dx),%al
    return -1;
  if(!(inb(COM1+5) & 0x01))
80105b9f:	a8 01                	test   $0x1,%al
80105ba1:	74 0d                	je     80105bb0 <uartgetc+0x20>
80105ba3:	ba f8 03 00 00       	mov    $0x3f8,%edx
80105ba8:	ec                   	in     (%dx),%al
    return -1;
  return inb(COM1+0);
80105ba9:	0f b6 c0             	movzbl %al,%eax
80105bac:	c3                   	ret
80105bad:	8d 76 00             	lea    0x0(%esi),%esi
    return -1;
80105bb0:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
}
80105bb5:	c3                   	ret
80105bb6:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80105bbd:	00 
80105bbe:	66 90                	xchg   %ax,%ax

80105bc0 <uartinit>:
{
80105bc0:	55                   	push   %ebp
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
80105bc1:	31 c9                	xor    %ecx,%ecx
80105bc3:	89 c8                	mov    %ecx,%eax
80105bc5:	89 e5                	mov    %esp,%ebp
80105bc7:	57                   	push   %edi
80105bc8:	bf fa 03 00 00       	mov    $0x3fa,%edi
80105bcd:	56                   	push   %esi
80105bce:	89 fa                	mov    %edi,%edx
80105bd0:	53                   	push   %ebx
80105bd1:	83 ec 1c             	sub    $0x1c,%esp
80105bd4:	ee                   	out    %al,(%dx)
80105bd5:	be fb 03 00 00       	mov    $0x3fb,%esi
80105bda:	b8 80 ff ff ff       	mov    $0xffffff80,%eax
80105bdf:	89 f2                	mov    %esi,%edx
80105be1:	ee                   	out    %al,(%dx)
80105be2:	b8 0c 00 00 00       	mov    $0xc,%eax
80105be7:	ba f8 03 00 00       	mov    $0x3f8,%edx
80105bec:	ee                   	out    %al,(%dx)
80105bed:	bb f9 03 00 00       	mov    $0x3f9,%ebx
80105bf2:	89 c8                	mov    %ecx,%eax
80105bf4:	89 da                	mov    %ebx,%edx
80105bf6:	ee                   	out    %al,(%dx)
80105bf7:	b8 03 00 00 00       	mov    $0x3,%eax
80105bfc:	89 f2                	mov    %esi,%edx
80105bfe:	ee                   	out    %al,(%dx)
80105bff:	ba fc 03 00 00       	mov    $0x3fc,%edx
80105c04:	89 c8                	mov    %ecx,%eax
80105c06:	ee                   	out    %al,(%dx)
80105c07:	b8 01 00 00 00       	mov    $0x1,%eax
80105c0c:	89 da                	mov    %ebx,%edx
80105c0e:	ee                   	out    %al,(%dx)
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
80105c0f:	ba fd 03 00 00       	mov    $0x3fd,%edx
80105c14:	ec                   	in     (%dx),%al
  if(inb(COM1+5) == 0xFF)
80105c15:	3c ff                	cmp    $0xff,%al
80105c17:	0f 84 7c 00 00 00    	je     80105c99 <uartinit+0xd9>
  uart = 1;
80105c1d:	c7 05 c0 44 11 80 01 	movl   $0x1,0x801144c0
80105c24:	00 00 00 
80105c27:	89 fa                	mov    %edi,%edx
80105c29:	ec                   	in     (%dx),%al
80105c2a:	ba f8 03 00 00       	mov    $0x3f8,%edx
80105c2f:	ec                   	in     (%dx),%al
  ioapicenable(IRQ_COM1, 0);
80105c30:	83 ec 08             	sub    $0x8,%esp
  for(p="xv6...\n"; *p; p++)
80105c33:	bf 58 75 10 80       	mov    $0x80107558,%edi
80105c38:	be fd 03 00 00       	mov    $0x3fd,%esi
  ioapicenable(IRQ_COM1, 0);
80105c3d:	6a 00                	push   $0x0
80105c3f:	6a 04                	push   $0x4
80105c41:	e8 1a c8 ff ff       	call   80102460 <ioapicenable>
80105c46:	83 c4 10             	add    $0x10,%esp
  for(p="xv6...\n"; *p; p++)
80105c49:	c6 45 e7 78          	movb   $0x78,-0x19(%ebp)
80105c4d:	8d 76 00             	lea    0x0(%esi),%esi
  if(!uart)
80105c50:	a1 c0 44 11 80       	mov    0x801144c0,%eax
80105c55:	85 c0                	test   %eax,%eax
80105c57:	74 32                	je     80105c8b <uartinit+0xcb>
80105c59:	89 f2                	mov    %esi,%edx
80105c5b:	ec                   	in     (%dx),%al
  for(i = 0; i < 128 && !(inb(COM1+5) & 0x20); i++)
80105c5c:	a8 20                	test   $0x20,%al
80105c5e:	75 21                	jne    80105c81 <uartinit+0xc1>
80105c60:	bb 80 00 00 00       	mov    $0x80,%ebx
80105c65:	8d 76 00             	lea    0x0(%esi),%esi
    microdelay(10);
80105c68:	83 ec 0c             	sub    $0xc,%esp
80105c6b:	6a 0a                	push   $0xa
80105c6d:	e8 9e cc ff ff       	call   80102910 <microdelay>
  for(i = 0; i < 128 && !(inb(COM1+5) & 0x20); i++)
80105c72:	83 c4 10             	add    $0x10,%esp
80105c75:	83 eb 01             	sub    $0x1,%ebx
80105c78:	74 07                	je     80105c81 <uartinit+0xc1>
80105c7a:	89 f2                	mov    %esi,%edx
80105c7c:	ec                   	in     (%dx),%al
80105c7d:	a8 20                	test   $0x20,%al
80105c7f:	74 e7                	je     80105c68 <uartinit+0xa8>
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
80105c81:	ba f8 03 00 00       	mov    $0x3f8,%edx
80105c86:	0f b6 45 e7          	movzbl -0x19(%ebp),%eax
80105c8a:	ee                   	out    %al,(%dx)
  for(p="xv6...\n"; *p; p++)
80105c8b:	0f b6 47 01          	movzbl 0x1(%edi),%eax
80105c8f:	83 c7 01             	add    $0x1,%edi
80105c92:	88 45 e7             	mov    %al,-0x19(%ebp)
80105c95:	84 c0                	test   %al,%al
80105c97:	75 b7                	jne    80105c50 <uartinit+0x90>
}
80105c99:	8d 65 f4             	lea    -0xc(%ebp),%esp
80105c9c:	5b                   	pop    %ebx
80105c9d:	5e                   	pop    %esi
80105c9e:	5f                   	pop    %edi
80105c9f:	5d                   	pop    %ebp
80105ca0:	c3                   	ret
80105ca1:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80105ca8:	00 
80105ca9:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi

80105cb0 <uartputc>:
  if(!uart)
80105cb0:	a1 c0 44 11 80       	mov    0x801144c0,%eax
80105cb5:	85 c0                	test   %eax,%eax
80105cb7:	74 4f                	je     80105d08 <uartputc+0x58>
{
80105cb9:	55                   	push   %ebp
  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
80105cba:	ba fd 03 00 00       	mov    $0x3fd,%edx
80105cbf:	89 e5                	mov    %esp,%ebp
80105cc1:	56                   	push   %esi
80105cc2:	53                   	push   %ebx
80105cc3:	ec                   	in     (%dx),%al
  for(i = 0; i < 128 && !(inb(COM1+5) & 0x20); i++)
80105cc4:	a8 20                	test   $0x20,%al
80105cc6:	75 29                	jne    80105cf1 <uartputc+0x41>
80105cc8:	bb 80 00 00 00       	mov    $0x80,%ebx
80105ccd:	be fd 03 00 00       	mov    $0x3fd,%esi
80105cd2:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    microdelay(10);
80105cd8:	83 ec 0c             	sub    $0xc,%esp
80105cdb:	6a 0a                	push   $0xa
80105cdd:	e8 2e cc ff ff       	call   80102910 <microdelay>
  for(i = 0; i < 128 && !(inb(COM1+5) & 0x20); i++)
80105ce2:	83 c4 10             	add    $0x10,%esp
80105ce5:	83 eb 01             	sub    $0x1,%ebx
80105ce8:	74 07                	je     80105cf1 <uartputc+0x41>
80105cea:	89 f2                	mov    %esi,%edx
80105cec:	ec                   	in     (%dx),%al
80105ced:	a8 20                	test   $0x20,%al
80105cef:	74 e7                	je     80105cd8 <uartputc+0x28>
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
80105cf1:	8b 45 08             	mov    0x8(%ebp),%eax
80105cf4:	ba f8 03 00 00       	mov    $0x3f8,%edx
80105cf9:	ee                   	out    %al,(%dx)
}
80105cfa:	8d 65 f8             	lea    -0x8(%ebp),%esp
80105cfd:	5b                   	pop    %ebx
80105cfe:	5e                   	pop    %esi
80105cff:	5d                   	pop    %ebp
80105d00:	c3                   	ret
80105d01:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
80105d08:	c3                   	ret
80105d09:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi

80105d10 <uartintr>:

void
uartintr(void)
{
80105d10:	55                   	push   %ebp
80105d11:	89 e5                	mov    %esp,%ebp
80105d13:	83 ec 14             	sub    $0x14,%esp
  consoleintr(uartgetc);
80105d16:	68 90 5b 10 80       	push   $0x80105b90
80105d1b:	e8 80 ab ff ff       	call   801008a0 <consoleintr>
}
80105d20:	83 c4 10             	add    $0x10,%esp
80105d23:	c9                   	leave
80105d24:	c3                   	ret

80105d25 <vector0>:
# generated by vectors.pl - do not edit
# handlers
.globl alltraps
.globl vector0
vector0:
  pushl $0
80105d25:	6a 00                	push   $0x0
  pushl $0
80105d27:	6a 00                	push   $0x0
  jmp alltraps
80105d29:	e9 0c fb ff ff       	jmp    8010583a <alltraps>

80105d2e <vector1>:
.globl vector1
vector1:
  pushl $0
80105d2e:	6a 00                	push   $0x0
  pushl $1
80105d30:	6a 01                	push   $0x1
  jmp alltraps
80105d32:	e9 03 fb ff ff       	jmp    8010583a <alltraps>

80105d37 <vector2>:
.globl vector2
vector2:
  pushl $0
80105d37:	6a 00                	push   $0x0
  pushl $2
80105d39:	6a 02                	push   $0x2
  jmp alltraps
80105d3b:	e9 fa fa ff ff       	jmp    8010583a <alltraps>

80105d40 <vector3>:
.globl vector3
vector3:
  pushl $0
80105d40:	6a 00                	push   $0x0
  pushl $3
80105d42:	6a 03                	push   $0x3
  jmp alltraps
80105d44:	e9 f1 fa ff ff       	jmp    8010583a <alltraps>

80105d49 <vector4>:
.globl vector4
vector4:
  pushl $0
80105d49:	6a 00                	push   $0x0
  pushl $4
80105d4b:	6a 04                	push   $0x4
  jmp alltraps
80105d4d:	e9 e8 fa ff ff       	jmp    8010583a <alltraps>

80105d52 <vector5>:
.globl vector5
vector5:
  pushl $0
80105d52:	6a 00                	push   $0x0
  pushl $5
80105d54:	6a 05                	push   $0x5
  jmp alltraps
80105d56:	e9 df fa ff ff       	jmp    8010583a <alltraps>

80105d5b <vector6>:
.globl vector6
vector6:
  pushl $0
80105d5b:	6a 00                	push   $0x0
  pushl $6
80105d5d:	6a 06                	push   $0x6
  jmp alltraps
80105d5f:	e9 d6 fa ff ff       	jmp    8010583a <alltraps>

80105d64 <vector7>:
.globl vector7
vector7:
  pushl $0
80105d64:	6a 00                	push   $0x0
  pushl $7
80105d66:	6a 07                	push   $0x7
  jmp alltraps
80105d68:	e9 cd fa ff ff       	jmp    8010583a <alltraps>

80105d6d <vector8>:
.globl vector8
vector8:
  pushl $8
80105d6d:	6a 08                	push   $0x8
  jmp alltraps
80105d6f:	e9 c6 fa ff ff       	jmp    8010583a <alltraps>

80105d74 <vector9>:
.globl vector9
vector9:
  pushl $0
80105d74:	6a 00                	push   $0x0
  pushl $9
80105d76:	6a 09                	push   $0x9
  jmp alltraps
80105d78:	e9 bd fa ff ff       	jmp    8010583a <alltraps>

80105d7d <vector10>:
.globl vector10
vector10:
  pushl $10
80105d7d:	6a 0a                	push   $0xa
  jmp alltraps
80105d7f:	e9 b6 fa ff ff       	jmp    8010583a <alltraps>

80105d84 <vector11>:
.globl vector11
vector11:
  pushl $11
80105d84:	6a 0b                	push   $0xb
  jmp alltraps
80105d86:	e9 af fa ff ff       	jmp    8010583a <alltraps>

80105d8b <vector12>:
.globl vector12
vector12:
  pushl $12
80105d8b:	6a 0c                	push   $0xc
  jmp alltraps
80105d8d:	e9 a8 fa ff ff       	jmp    8010583a <alltraps>

80105d92 <vector13>:
.globl vector13
vector13:
  pushl $13
80105d92:	6a 0d                	push   $0xd
  jmp alltraps
80105d94:	e9 a1 fa ff ff       	jmp    8010583a <alltraps>

80105d99 <vector14>:
.globl vector14
vector14:
  pushl $14
80105d99:	6a 0e                	push   $0xe
  jmp alltraps
80105d9b:	e9 9a fa ff ff       	jmp    8010583a <alltraps>

80105da0 <vector15>:
.globl vector15
vector15:
  pushl $0
80105da0:	6a 00                	push   $0x0
  pushl $15
80105da2:	6a 0f                	push   $0xf
  jmp alltraps
80105da4:	e9 91 fa ff ff       	jmp    8010583a <alltraps>

80105da9 <vector16>:
.globl vector16
vector16:
  pushl $0
80105da9:	6a 00                	push   $0x0
  pushl $16
80105dab:	6a 10                	push   $0x10
  jmp alltraps
80105dad:	e9 88 fa ff ff       	jmp    8010583a <alltraps>

80105db2 <vector17>:
.globl vector17
vector17:
  pushl $17
80105db2:	6a 11                	push   $0x11
  jmp alltraps
80105db4:	e9 81 fa ff ff       	jmp    8010583a <alltraps>

80105db9 <vector18>:
.globl vector18
vector18:
  pushl $0
80105db9:	6a 00                	push   $0x0
  pushl $18
80105dbb:	6a 12                	push   $0x12
  jmp alltraps
80105dbd:	e9 78 fa ff ff       	jmp    8010583a <alltraps>

80105dc2 <vector19>:
.globl vector19
vector19:
  pushl $0
80105dc2:	6a 00                	push   $0x0
  pushl $19
80105dc4:	6a 13                	push   $0x13
  jmp alltraps
80105dc6:	e9 6f fa ff ff       	jmp    8010583a <alltraps>

80105dcb <vector20>:
.globl vector20
vector20:
  pushl $0
80105dcb:	6a 00                	push   $0x0
  pushl $20
80105dcd:	6a 14                	push   $0x14
  jmp alltraps
80105dcf:	e9 66 fa ff ff       	jmp    8010583a <alltraps>

80105dd4 <vector21>:
.globl vector21
vector21:
  pushl $0
80105dd4:	6a 00                	push   $0x0
  pushl $21
80105dd6:	6a 15                	push   $0x15
  jmp alltraps
80105dd8:	e9 5d fa ff ff       	jmp    8010583a <alltraps>

80105ddd <vector22>:
.globl vector22
vector22:
  pushl $0
80105ddd:	6a 00                	push   $0x0
  pushl $22
80105ddf:	6a 16                	push   $0x16
  jmp alltraps
80105de1:	e9 54 fa ff ff       	jmp    8010583a <alltraps>

80105de6 <vector23>:
.globl vector23
vector23:
  pushl $0
80105de6:	6a 00                	push   $0x0
  pushl $23
80105de8:	6a 17                	push   $0x17
  jmp alltraps
80105dea:	e9 4b fa ff ff       	jmp    8010583a <alltraps>

80105def <vector24>:
.globl vector24
vector24:
  pushl $0
80105def:	6a 00                	push   $0x0
  pushl $24
80105df1:	6a 18                	push   $0x18
  jmp alltraps
80105df3:	e9 42 fa ff ff       	jmp    8010583a <alltraps>

80105df8 <vector25>:
.globl vector25
vector25:
  pushl $0
80105df8:	6a 00                	push   $0x0
  pushl $25
80105dfa:	6a 19                	push   $0x19
  jmp alltraps
80105dfc:	e9 39 fa ff ff       	jmp    8010583a <alltraps>

80105e01 <vector26>:
.globl vector26
vector26:
  pushl $0
80105e01:	6a 00                	push   $0x0
  pushl $26
80105e03:	6a 1a                	push   $0x1a
  jmp alltraps
80105e05:	e9 30 fa ff ff       	jmp    8010583a <alltraps>

80105e0a <vector27>:
.globl vector27
vector27:
  pushl $0
80105e0a:	6a 00                	push   $0x0
  pushl $27
80105e0c:	6a 1b                	push   $0x1b
  jmp alltraps
80105e0e:	e9 27 fa ff ff       	jmp    8010583a <alltraps>

80105e13 <vector28>:
.globl vector28
vector28:
  pushl $0
80105e13:	6a 00                	push   $0x0
  pushl $28
80105e15:	6a 1c                	push   $0x1c
  jmp alltraps
80105e17:	e9 1e fa ff ff       	jmp    8010583a <alltraps>

80105e1c <vector29>:
.globl vector29
vector29:
  pushl $0
80105e1c:	6a 00                	push   $0x0
  pushl $29
80105e1e:	6a 1d                	push   $0x1d
  jmp alltraps
80105e20:	e9 15 fa ff ff       	jmp    8010583a <alltraps>

80105e25 <vector30>:
.globl vector30
vector30:
  pushl $0
80105e25:	6a 00                	push   $0x0
  pushl $30
80105e27:	6a 1e                	push   $0x1e
  jmp alltraps
80105e29:	e9 0c fa ff ff       	jmp    8010583a <alltraps>

80105e2e <vector31>:
.globl vector31
vector31:
  pushl $0
80105e2e:	6a 00                	push   $0x0
  pushl $31
80105e30:	6a 1f                	push   $0x1f
  jmp alltraps
80105e32:	e9 03 fa ff ff       	jmp    8010583a <alltraps>

80105e37 <vector32>:
.globl vector32
vector32:
  pushl $0
80105e37:	6a 00                	push   $0x0
  pushl $32
80105e39:	6a 20                	push   $0x20
  jmp alltraps
80105e3b:	e9 fa f9 ff ff       	jmp    8010583a <alltraps>

80105e40 <vector33>:
.globl vector33
vector33:
  pushl $0
80105e40:	6a 00                	push   $0x0
  pushl $33
80105e42:	6a 21                	push   $0x21
  jmp alltraps
80105e44:	e9 f1 f9 ff ff       	jmp    8010583a <alltraps>

80105e49 <vector34>:
.globl vector34
vector34:
  pushl $0
80105e49:	6a 00                	push   $0x0
  pushl $34
80105e4b:	6a 22                	push   $0x22
  jmp alltraps
80105e4d:	e9 e8 f9 ff ff       	jmp    8010583a <alltraps>

80105e52 <vector35>:
.globl vector35
vector35:
  pushl $0
80105e52:	6a 00                	push   $0x0
  pushl $35
80105e54:	6a 23                	push   $0x23
  jmp alltraps
80105e56:	e9 df f9 ff ff       	jmp    8010583a <alltraps>

80105e5b <vector36>:
.globl vector36
vector36:
  pushl $0
80105e5b:	6a 00                	push   $0x0
  pushl $36
80105e5d:	6a 24                	push   $0x24
  jmp alltraps
80105e5f:	e9 d6 f9 ff ff       	jmp    8010583a <alltraps>

80105e64 <vector37>:
.globl vector37
vector37:
  pushl $0
80105e64:	6a 00                	push   $0x0
  pushl $37
80105e66:	6a 25                	push   $0x25
  jmp alltraps
80105e68:	e9 cd f9 ff ff       	jmp    8010583a <alltraps>

80105e6d <vector38>:
.globl vector38
vector38:
  pushl $0
80105e6d:	6a 00                	push   $0x0
  pushl $38
80105e6f:	6a 26                	push   $0x26
  jmp alltraps
80105e71:	e9 c4 f9 ff ff       	jmp    8010583a <alltraps>

80105e76 <vector39>:
.globl vector39
vector39:
  pushl $0
80105e76:	6a 00                	push   $0x0
  pushl $39
80105e78:	6a 27                	push   $0x27
  jmp alltraps
80105e7a:	e9 bb f9 ff ff       	jmp    8010583a <alltraps>

80105e7f <vector40>:
.globl vector40
vector40:
  pushl $0
80105e7f:	6a 00                	push   $0x0
  pushl $40
80105e81:	6a 28                	push   $0x28
  jmp alltraps
80105e83:	e9 b2 f9 ff ff       	jmp    8010583a <alltraps>

80105e88 <vector41>:
.globl vector41
vector41:
  pushl $0
80105e88:	6a 00                	push   $0x0
  pushl $41
80105e8a:	6a 29                	push   $0x29
  jmp alltraps
80105e8c:	e9 a9 f9 ff ff       	jmp    8010583a <alltraps>

80105e91 <vector42>:
.globl vector42
vector42:
  pushl $0
80105e91:	6a 00                	push   $0x0
  pushl $42
80105e93:	6a 2a                	push   $0x2a
  jmp alltraps
80105e95:	e9 a0 f9 ff ff       	jmp    8010583a <alltraps>

80105e9a <vector43>:
.globl vector43
vector43:
  pushl $0
80105e9a:	6a 00                	push   $0x0
  pushl $43
80105e9c:	6a 2b                	push   $0x2b
  jmp alltraps
80105e9e:	e9 97 f9 ff ff       	jmp    8010583a <alltraps>

80105ea3 <vector44>:
.globl vector44
vector44:
  pushl $0
80105ea3:	6a 00                	push   $0x0
  pushl $44
80105ea5:	6a 2c                	push   $0x2c
  jmp alltraps
80105ea7:	e9 8e f9 ff ff       	jmp    8010583a <alltraps>

80105eac <vector45>:
.globl vector45
vector45:
  pushl $0
80105eac:	6a 00                	push   $0x0
  pushl $45
80105eae:	6a 2d                	push   $0x2d
  jmp alltraps
80105eb0:	e9 85 f9 ff ff       	jmp    8010583a <alltraps>

80105eb5 <vector46>:
.globl vector46
vector46:
  pushl $0
80105eb5:	6a 00                	push   $0x0
  pushl $46
80105eb7:	6a 2e                	push   $0x2e
  jmp alltraps
80105eb9:	e9 7c f9 ff ff       	jmp    8010583a <alltraps>

80105ebe <vector47>:
.globl vector47
vector47:
  pushl $0
80105ebe:	6a 00                	push   $0x0
  pushl $47
80105ec0:	6a 2f                	push   $0x2f
  jmp alltraps
80105ec2:	e9 73 f9 ff ff       	jmp    8010583a <alltraps>

80105ec7 <vector48>:
.globl vector48
vector48:
  pushl $0
80105ec7:	6a 00                	push   $0x0
  pushl $48
80105ec9:	6a 30                	push   $0x30
  jmp alltraps
80105ecb:	e9 6a f9 ff ff       	jmp    8010583a <alltraps>

80105ed0 <vector49>:
.globl vector49
vector49:
  pushl $0
80105ed0:	6a 00                	push   $0x0
  pushl $49
80105ed2:	6a 31                	push   $0x31
  jmp alltraps
80105ed4:	e9 61 f9 ff ff       	jmp    8010583a <alltraps>

80105ed9 <vector50>:
.globl vector50
vector50:
  pushl $0
80105ed9:	6a 00                	push   $0x0
  pushl $50
80105edb:	6a 32                	push   $0x32
  jmp alltraps
80105edd:	e9 58 f9 ff ff       	jmp    8010583a <alltraps>

80105ee2 <vector51>:
.globl vector51
vector51:
  pushl $0
80105ee2:	6a 00                	push   $0x0
  pushl $51
80105ee4:	6a 33                	push   $0x33
  jmp alltraps
80105ee6:	e9 4f f9 ff ff       	jmp    8010583a <alltraps>

80105eeb <vector52>:
.globl vector52
vector52:
  pushl $0
80105eeb:	6a 00                	push   $0x0
  pushl $52
80105eed:	6a 34                	push   $0x34
  jmp alltraps
80105eef:	e9 46 f9 ff ff       	jmp    8010583a <alltraps>

80105ef4 <vector53>:
.globl vector53
vector53:
  pushl $0
80105ef4:	6a 00                	push   $0x0
  pushl $53
80105ef6:	6a 35                	push   $0x35
  jmp alltraps
80105ef8:	e9 3d f9 ff ff       	jmp    8010583a <alltraps>

80105efd <vector54>:
.globl vector54
vector54:
  pushl $0
80105efd:	6a 00                	push   $0x0
  pushl $54
80105eff:	6a 36                	push   $0x36
  jmp alltraps
80105f01:	e9 34 f9 ff ff       	jmp    8010583a <alltraps>

80105f06 <vector55>:
.globl vector55
vector55:
  pushl $0
80105f06:	6a 00                	push   $0x0
  pushl $55
80105f08:	6a 37                	push   $0x37
  jmp alltraps
80105f0a:	e9 2b f9 ff ff       	jmp    8010583a <alltraps>

80105f0f <vector56>:
.globl vector56
vector56:
  pushl $0
80105f0f:	6a 00                	push   $0x0
  pushl $56
80105f11:	6a 38                	push   $0x38
  jmp alltraps
80105f13:	e9 22 f9 ff ff       	jmp    8010583a <alltraps>

80105f18 <vector57>:
.globl vector57
vector57:
  pushl $0
80105f18:	6a 00                	push   $0x0
  pushl $57
80105f1a:	6a 39                	push   $0x39
  jmp alltraps
80105f1c:	e9 19 f9 ff ff       	jmp    8010583a <alltraps>

80105f21 <vector58>:
.globl vector58
vector58:
  pushl $0
80105f21:	6a 00                	push   $0x0
  pushl $58
80105f23:	6a 3a                	push   $0x3a
  jmp alltraps
80105f25:	e9 10 f9 ff ff       	jmp    8010583a <alltraps>

80105f2a <vector59>:
.globl vector59
vector59:
  pushl $0
80105f2a:	6a 00                	push   $0x0
  pushl $59
80105f2c:	6a 3b                	push   $0x3b
  jmp alltraps
80105f2e:	e9 07 f9 ff ff       	jmp    8010583a <alltraps>

80105f33 <vector60>:
.globl vector60
vector60:
  pushl $0
80105f33:	6a 00                	push   $0x0
  pushl $60
80105f35:	6a 3c                	push   $0x3c
  jmp alltraps
80105f37:	e9 fe f8 ff ff       	jmp    8010583a <alltraps>

80105f3c <vector61>:
.globl vector61
vector61:
  pushl $0
80105f3c:	6a 00                	push   $0x0
  pushl $61
80105f3e:	6a 3d                	push   $0x3d
  jmp alltraps
80105f40:	e9 f5 f8 ff ff       	jmp    8010583a <alltraps>

80105f45 <vector62>:
.globl vector62
vector62:
  pushl $0
80105f45:	6a 00                	push   $0x0
  pushl $62
80105f47:	6a 3e                	push   $0x3e
  jmp alltraps
80105f49:	e9 ec f8 ff ff       	jmp    8010583a <alltraps>

80105f4e <vector63>:
.globl vector63
vector63:
  pushl $0
80105f4e:	6a 00                	push   $0x0
  pushl $63
80105f50:	6a 3f                	push   $0x3f
  jmp alltraps
80105f52:	e9 e3 f8 ff ff       	jmp    8010583a <alltraps>

80105f57 <vector64>:
.globl vector64
vector64:
  pushl $0
80105f57:	6a 00                	push   $0x0
  pushl $64
80105f59:	6a 40                	push   $0x40
  jmp alltraps
80105f5b:	e9 da f8 ff ff       	jmp    8010583a <alltraps>

80105f60 <vector65>:
.globl vector65
vector65:
  pushl $0
80105f60:	6a 00                	push   $0x0
  pushl $65
80105f62:	6a 41                	push   $0x41
  jmp alltraps
80105f64:	e9 d1 f8 ff ff       	jmp    8010583a <alltraps>

80105f69 <vector66>:
.globl vector66
vector66:
  pushl $0
80105f69:	6a 00                	push   $0x0
  pushl $66
80105f6b:	6a 42                	push   $0x42
  jmp alltraps
80105f6d:	e9 c8 f8 ff ff       	jmp    8010583a <alltraps>

80105f72 <vector67>:
.globl vector67
vector67:
  pushl $0
80105f72:	6a 00                	push   $0x0
  pushl $67
80105f74:	6a 43                	push   $0x43
  jmp alltraps
80105f76:	e9 bf f8 ff ff       	jmp    8010583a <alltraps>

80105f7b <vector68>:
.globl vector68
vector68:
  pushl $0
80105f7b:	6a 00                	push   $0x0
  pushl $68
80105f7d:	6a 44                	push   $0x44
  jmp alltraps
80105f7f:	e9 b6 f8 ff ff       	jmp    8010583a <alltraps>

80105f84 <vector69>:
.globl vector69
vector69:
  pushl $0
80105f84:	6a 00                	push   $0x0
  pushl $69
80105f86:	6a 45                	push   $0x45
  jmp alltraps
80105f88:	e9 ad f8 ff ff       	jmp    8010583a <alltraps>

80105f8d <vector70>:
.globl vector70
vector70:
  pushl $0
80105f8d:	6a 00                	push   $0x0
  pushl $70
80105f8f:	6a 46                	push   $0x46
  jmp alltraps
80105f91:	e9 a4 f8 ff ff       	jmp    8010583a <alltraps>

80105f96 <vector71>:
.globl vector71
vector71:
  pushl $0
80105f96:	6a 00                	push   $0x0
  pushl $71
80105f98:	6a 47                	push   $0x47
  jmp alltraps
80105f9a:	e9 9b f8 ff ff       	jmp    8010583a <alltraps>

80105f9f <vector72>:
.globl vector72
vector72:
  pushl $0
80105f9f:	6a 00                	push   $0x0
  pushl $72
80105fa1:	6a 48                	push   $0x48
  jmp alltraps
80105fa3:	e9 92 f8 ff ff       	jmp    8010583a <alltraps>

80105fa8 <vector73>:
.globl vector73
vector73:
  pushl $0
80105fa8:	6a 00                	push   $0x0
  pushl $73
80105faa:	6a 49                	push   $0x49
  jmp alltraps
80105fac:	e9 89 f8 ff ff       	jmp    8010583a <alltraps>

80105fb1 <vector74>:
.globl vector74
vector74:
  pushl $0
80105fb1:	6a 00                	push   $0x0
  pushl $74
80105fb3:	6a 4a                	push   $0x4a
  jmp alltraps
80105fb5:	e9 80 f8 ff ff       	jmp    8010583a <alltraps>

80105fba <vector75>:
.globl vector75
vector75:
  pushl $0
80105fba:	6a 00                	push   $0x0
  pushl $75
80105fbc:	6a 4b                	push   $0x4b
  jmp alltraps
80105fbe:	e9 77 f8 ff ff       	jmp    8010583a <alltraps>

80105fc3 <vector76>:
.globl vector76
vector76:
  pushl $0
80105fc3:	6a 00                	push   $0x0
  pushl $76
80105fc5:	6a 4c                	push   $0x4c
  jmp alltraps
80105fc7:	e9 6e f8 ff ff       	jmp    8010583a <alltraps>

80105fcc <vector77>:
.globl vector77
vector77:
  pushl $0
80105fcc:	6a 00                	push   $0x0
  pushl $77
80105fce:	6a 4d                	push   $0x4d
  jmp alltraps
80105fd0:	e9 65 f8 ff ff       	jmp    8010583a <alltraps>

80105fd5 <vector78>:
.globl vector78
vector78:
  pushl $0
80105fd5:	6a 00                	push   $0x0
  pushl $78
80105fd7:	6a 4e                	push   $0x4e
  jmp alltraps
80105fd9:	e9 5c f8 ff ff       	jmp    8010583a <alltraps>

80105fde <vector79>:
.globl vector79
vector79:
  pushl $0
80105fde:	6a 00                	push   $0x0
  pushl $79
80105fe0:	6a 4f                	push   $0x4f
  jmp alltraps
80105fe2:	e9 53 f8 ff ff       	jmp    8010583a <alltraps>

80105fe7 <vector80>:
.globl vector80
vector80:
  pushl $0
80105fe7:	6a 00                	push   $0x0
  pushl $80
80105fe9:	6a 50                	push   $0x50
  jmp alltraps
80105feb:	e9 4a f8 ff ff       	jmp    8010583a <alltraps>

80105ff0 <vector81>:
.globl vector81
vector81:
  pushl $0
80105ff0:	6a 00                	push   $0x0
  pushl $81
80105ff2:	6a 51                	push   $0x51
  jmp alltraps
80105ff4:	e9 41 f8 ff ff       	jmp    8010583a <alltraps>

80105ff9 <vector82>:
.globl vector82
vector82:
  pushl $0
80105ff9:	6a 00                	push   $0x0
  pushl $82
80105ffb:	6a 52                	push   $0x52
  jmp alltraps
80105ffd:	e9 38 f8 ff ff       	jmp    8010583a <alltraps>

80106002 <vector83>:
.globl vector83
vector83:
  pushl $0
80106002:	6a 00                	push   $0x0
  pushl $83
80106004:	6a 53                	push   $0x53
  jmp alltraps
80106006:	e9 2f f8 ff ff       	jmp    8010583a <alltraps>

8010600b <vector84>:
.globl vector84
vector84:
  pushl $0
8010600b:	6a 00                	push   $0x0
  pushl $84
8010600d:	6a 54                	push   $0x54
  jmp alltraps
8010600f:	e9 26 f8 ff ff       	jmp    8010583a <alltraps>

80106014 <vector85>:
.globl vector85
vector85:
  pushl $0
80106014:	6a 00                	push   $0x0
  pushl $85
80106016:	6a 55                	push   $0x55
  jmp alltraps
80106018:	e9 1d f8 ff ff       	jmp    8010583a <alltraps>

8010601d <vector86>:
.globl vector86
vector86:
  pushl $0
8010601d:	6a 00                	push   $0x0
  pushl $86
8010601f:	6a 56                	push   $0x56
  jmp alltraps
80106021:	e9 14 f8 ff ff       	jmp    8010583a <alltraps>

80106026 <vector87>:
.globl vector87
vector87:
  pushl $0
80106026:	6a 00                	push   $0x0
  pushl $87
80106028:	6a 57                	push   $0x57
  jmp alltraps
8010602a:	e9 0b f8 ff ff       	jmp    8010583a <alltraps>

8010602f <vector88>:
.globl vector88
vector88:
  pushl $0
8010602f:	6a 00                	push   $0x0
  pushl $88
80106031:	6a 58                	push   $0x58
  jmp alltraps
80106033:	e9 02 f8 ff ff       	jmp    8010583a <alltraps>

80106038 <vector89>:
.globl vector89
vector89:
  pushl $0
80106038:	6a 00                	push   $0x0
  pushl $89
8010603a:	6a 59                	push   $0x59
  jmp alltraps
8010603c:	e9 f9 f7 ff ff       	jmp    8010583a <alltraps>

80106041 <vector90>:
.globl vector90
vector90:
  pushl $0
80106041:	6a 00                	push   $0x0
  pushl $90
80106043:	6a 5a                	push   $0x5a
  jmp alltraps
80106045:	e9 f0 f7 ff ff       	jmp    8010583a <alltraps>

8010604a <vector91>:
.globl vector91
vector91:
  pushl $0
8010604a:	6a 00                	push   $0x0
  pushl $91
8010604c:	6a 5b                	push   $0x5b
  jmp alltraps
8010604e:	e9 e7 f7 ff ff       	jmp    8010583a <alltraps>

80106053 <vector92>:
.globl vector92
vector92:
  pushl $0
80106053:	6a 00                	push   $0x0
  pushl $92
80106055:	6a 5c                	push   $0x5c
  jmp alltraps
80106057:	e9 de f7 ff ff       	jmp    8010583a <alltraps>

8010605c <vector93>:
.globl vector93
vector93:
  pushl $0
8010605c:	6a 00                	push   $0x0
  pushl $93
8010605e:	6a 5d                	push   $0x5d
  jmp alltraps
80106060:	e9 d5 f7 ff ff       	jmp    8010583a <alltraps>

80106065 <vector94>:
.globl vector94
vector94:
  pushl $0
80106065:	6a 00                	push   $0x0
  pushl $94
80106067:	6a 5e                	push   $0x5e
  jmp alltraps
80106069:	e9 cc f7 ff ff       	jmp    8010583a <alltraps>

8010606e <vector95>:
.globl vector95
vector95:
  pushl $0
8010606e:	6a 00                	push   $0x0
  pushl $95
80106070:	6a 5f                	push   $0x5f
  jmp alltraps
80106072:	e9 c3 f7 ff ff       	jmp    8010583a <alltraps>

80106077 <vector96>:
.globl vector96
vector96:
  pushl $0
80106077:	6a 00                	push   $0x0
  pushl $96
80106079:	6a 60                	push   $0x60
  jmp alltraps
8010607b:	e9 ba f7 ff ff       	jmp    8010583a <alltraps>

80106080 <vector97>:
.globl vector97
vector97:
  pushl $0
80106080:	6a 00                	push   $0x0
  pushl $97
80106082:	6a 61                	push   $0x61
  jmp alltraps
80106084:	e9 b1 f7 ff ff       	jmp    8010583a <alltraps>

80106089 <vector98>:
.globl vector98
vector98:
  pushl $0
80106089:	6a 00                	push   $0x0
  pushl $98
8010608b:	6a 62                	push   $0x62
  jmp alltraps
8010608d:	e9 a8 f7 ff ff       	jmp    8010583a <alltraps>

80106092 <vector99>:
.globl vector99
vector99:
  pushl $0
80106092:	6a 00                	push   $0x0
  pushl $99
80106094:	6a 63                	push   $0x63
  jmp alltraps
80106096:	e9 9f f7 ff ff       	jmp    8010583a <alltraps>

8010609b <vector100>:
.globl vector100
vector100:
  pushl $0
8010609b:	6a 00                	push   $0x0
  pushl $100
8010609d:	6a 64                	push   $0x64
  jmp alltraps
8010609f:	e9 96 f7 ff ff       	jmp    8010583a <alltraps>

801060a4 <vector101>:
.globl vector101
vector101:
  pushl $0
801060a4:	6a 00                	push   $0x0
  pushl $101
801060a6:	6a 65                	push   $0x65
  jmp alltraps
801060a8:	e9 8d f7 ff ff       	jmp    8010583a <alltraps>

801060ad <vector102>:
.globl vector102
vector102:
  pushl $0
801060ad:	6a 00                	push   $0x0
  pushl $102
801060af:	6a 66                	push   $0x66
  jmp alltraps
801060b1:	e9 84 f7 ff ff       	jmp    8010583a <alltraps>

801060b6 <vector103>:
.globl vector103
vector103:
  pushl $0
801060b6:	6a 00                	push   $0x0
  pushl $103
801060b8:	6a 67                	push   $0x67
  jmp alltraps
801060ba:	e9 7b f7 ff ff       	jmp    8010583a <alltraps>

801060bf <vector104>:
.globl vector104
vector104:
  pushl $0
801060bf:	6a 00                	push   $0x0
  pushl $104
801060c1:	6a 68                	push   $0x68
  jmp alltraps
801060c3:	e9 72 f7 ff ff       	jmp    8010583a <alltraps>

801060c8 <vector105>:
.globl vector105
vector105:
  pushl $0
801060c8:	6a 00                	push   $0x0
  pushl $105
801060ca:	6a 69                	push   $0x69
  jmp alltraps
801060cc:	e9 69 f7 ff ff       	jmp    8010583a <alltraps>

801060d1 <vector106>:
.globl vector106
vector106:
  pushl $0
801060d1:	6a 00                	push   $0x0
  pushl $106
801060d3:	6a 6a                	push   $0x6a
  jmp alltraps
801060d5:	e9 60 f7 ff ff       	jmp    8010583a <alltraps>

801060da <vector107>:
.globl vector107
vector107:
  pushl $0
801060da:	6a 00                	push   $0x0
  pushl $107
801060dc:	6a 6b                	push   $0x6b
  jmp alltraps
801060de:	e9 57 f7 ff ff       	jmp    8010583a <alltraps>

801060e3 <vector108>:
.globl vector108
vector108:
  pushl $0
801060e3:	6a 00                	push   $0x0
  pushl $108
801060e5:	6a 6c                	push   $0x6c
  jmp alltraps
801060e7:	e9 4e f7 ff ff       	jmp    8010583a <alltraps>

801060ec <vector109>:
.globl vector109
vector109:
  pushl $0
801060ec:	6a 00                	push   $0x0
  pushl $109
801060ee:	6a 6d                	push   $0x6d
  jmp alltraps
801060f0:	e9 45 f7 ff ff       	jmp    8010583a <alltraps>

801060f5 <vector110>:
.globl vector110
vector110:
  pushl $0
801060f5:	6a 00                	push   $0x0
  pushl $110
801060f7:	6a 6e                	push   $0x6e
  jmp alltraps
801060f9:	e9 3c f7 ff ff       	jmp    8010583a <alltraps>

801060fe <vector111>:
.globl vector111
vector111:
  pushl $0
801060fe:	6a 00                	push   $0x0
  pushl $111
80106100:	6a 6f                	push   $0x6f
  jmp alltraps
80106102:	e9 33 f7 ff ff       	jmp    8010583a <alltraps>

80106107 <vector112>:
.globl vector112
vector112:
  pushl $0
80106107:	6a 00                	push   $0x0
  pushl $112
80106109:	6a 70                	push   $0x70
  jmp alltraps
8010610b:	e9 2a f7 ff ff       	jmp    8010583a <alltraps>

80106110 <vector113>:
.globl vector113
vector113:
  pushl $0
80106110:	6a 00                	push   $0x0
  pushl $113
80106112:	6a 71                	push   $0x71
  jmp alltraps
80106114:	e9 21 f7 ff ff       	jmp    8010583a <alltraps>

80106119 <vector114>:
.globl vector114
vector114:
  pushl $0
80106119:	6a 00                	push   $0x0
  pushl $114
8010611b:	6a 72                	push   $0x72
  jmp alltraps
8010611d:	e9 18 f7 ff ff       	jmp    8010583a <alltraps>

80106122 <vector115>:
.globl vector115
vector115:
  pushl $0
80106122:	6a 00                	push   $0x0
  pushl $115
80106124:	6a 73                	push   $0x73
  jmp alltraps
80106126:	e9 0f f7 ff ff       	jmp    8010583a <alltraps>

8010612b <vector116>:
.globl vector116
vector116:
  pushl $0
8010612b:	6a 00                	push   $0x0
  pushl $116
8010612d:	6a 74                	push   $0x74
  jmp alltraps
8010612f:	e9 06 f7 ff ff       	jmp    8010583a <alltraps>

80106134 <vector117>:
.globl vector117
vector117:
  pushl $0
80106134:	6a 00                	push   $0x0
  pushl $117
80106136:	6a 75                	push   $0x75
  jmp alltraps
80106138:	e9 fd f6 ff ff       	jmp    8010583a <alltraps>

8010613d <vector118>:
.globl vector118
vector118:
  pushl $0
8010613d:	6a 00                	push   $0x0
  pushl $118
8010613f:	6a 76                	push   $0x76
  jmp alltraps
80106141:	e9 f4 f6 ff ff       	jmp    8010583a <alltraps>

80106146 <vector119>:
.globl vector119
vector119:
  pushl $0
80106146:	6a 00                	push   $0x0
  pushl $119
80106148:	6a 77                	push   $0x77
  jmp alltraps
8010614a:	e9 eb f6 ff ff       	jmp    8010583a <alltraps>

8010614f <vector120>:
.globl vector120
vector120:
  pushl $0
8010614f:	6a 00                	push   $0x0
  pushl $120
80106151:	6a 78                	push   $0x78
  jmp alltraps
80106153:	e9 e2 f6 ff ff       	jmp    8010583a <alltraps>

80106158 <vector121>:
.globl vector121
vector121:
  pushl $0
80106158:	6a 00                	push   $0x0
  pushl $121
8010615a:	6a 79                	push   $0x79
  jmp alltraps
8010615c:	e9 d9 f6 ff ff       	jmp    8010583a <alltraps>

80106161 <vector122>:
.globl vector122
vector122:
  pushl $0
80106161:	6a 00                	push   $0x0
  pushl $122
80106163:	6a 7a                	push   $0x7a
  jmp alltraps
80106165:	e9 d0 f6 ff ff       	jmp    8010583a <alltraps>

8010616a <vector123>:
.globl vector123
vector123:
  pushl $0
8010616a:	6a 00                	push   $0x0
  pushl $123
8010616c:	6a 7b                	push   $0x7b
  jmp alltraps
8010616e:	e9 c7 f6 ff ff       	jmp    8010583a <alltraps>

80106173 <vector124>:
.globl vector124
vector124:
  pushl $0
80106173:	6a 00                	push   $0x0
  pushl $124
80106175:	6a 7c                	push   $0x7c
  jmp alltraps
80106177:	e9 be f6 ff ff       	jmp    8010583a <alltraps>

8010617c <vector125>:
.globl vector125
vector125:
  pushl $0
8010617c:	6a 00                	push   $0x0
  pushl $125
8010617e:	6a 7d                	push   $0x7d
  jmp alltraps
80106180:	e9 b5 f6 ff ff       	jmp    8010583a <alltraps>

80106185 <vector126>:
.globl vector126
vector126:
  pushl $0
80106185:	6a 00                	push   $0x0
  pushl $126
80106187:	6a 7e                	push   $0x7e
  jmp alltraps
80106189:	e9 ac f6 ff ff       	jmp    8010583a <alltraps>

8010618e <vector127>:
.globl vector127
vector127:
  pushl $0
8010618e:	6a 00                	push   $0x0
  pushl $127
80106190:	6a 7f                	push   $0x7f
  jmp alltraps
80106192:	e9 a3 f6 ff ff       	jmp    8010583a <alltraps>

80106197 <vector128>:
.globl vector128
vector128:
  pushl $0
80106197:	6a 00                	push   $0x0
  pushl $128
80106199:	68 80 00 00 00       	push   $0x80
  jmp alltraps
8010619e:	e9 97 f6 ff ff       	jmp    8010583a <alltraps>

801061a3 <vector129>:
.globl vector129
vector129:
  pushl $0
801061a3:	6a 00                	push   $0x0
  pushl $129
801061a5:	68 81 00 00 00       	push   $0x81
  jmp alltraps
801061aa:	e9 8b f6 ff ff       	jmp    8010583a <alltraps>

801061af <vector130>:
.globl vector130
vector130:
  pushl $0
801061af:	6a 00                	push   $0x0
  pushl $130
801061b1:	68 82 00 00 00       	push   $0x82
  jmp alltraps
801061b6:	e9 7f f6 ff ff       	jmp    8010583a <alltraps>

801061bb <vector131>:
.globl vector131
vector131:
  pushl $0
801061bb:	6a 00                	push   $0x0
  pushl $131
801061bd:	68 83 00 00 00       	push   $0x83
  jmp alltraps
801061c2:	e9 73 f6 ff ff       	jmp    8010583a <alltraps>

801061c7 <vector132>:
.globl vector132
vector132:
  pushl $0
801061c7:	6a 00                	push   $0x0
  pushl $132
801061c9:	68 84 00 00 00       	push   $0x84
  jmp alltraps
801061ce:	e9 67 f6 ff ff       	jmp    8010583a <alltraps>

801061d3 <vector133>:
.globl vector133
vector133:
  pushl $0
801061d3:	6a 00                	push   $0x0
  pushl $133
801061d5:	68 85 00 00 00       	push   $0x85
  jmp alltraps
801061da:	e9 5b f6 ff ff       	jmp    8010583a <alltraps>

801061df <vector134>:
.globl vector134
vector134:
  pushl $0
801061df:	6a 00                	push   $0x0
  pushl $134
801061e1:	68 86 00 00 00       	push   $0x86
  jmp alltraps
801061e6:	e9 4f f6 ff ff       	jmp    8010583a <alltraps>

801061eb <vector135>:
.globl vector135
vector135:
  pushl $0
801061eb:	6a 00                	push   $0x0
  pushl $135
801061ed:	68 87 00 00 00       	push   $0x87
  jmp alltraps
801061f2:	e9 43 f6 ff ff       	jmp    8010583a <alltraps>

801061f7 <vector136>:
.globl vector136
vector136:
  pushl $0
801061f7:	6a 00                	push   $0x0
  pushl $136
801061f9:	68 88 00 00 00       	push   $0x88
  jmp alltraps
801061fe:	e9 37 f6 ff ff       	jmp    8010583a <alltraps>

80106203 <vector137>:
.globl vector137
vector137:
  pushl $0
80106203:	6a 00                	push   $0x0
  pushl $137
80106205:	68 89 00 00 00       	push   $0x89
  jmp alltraps
8010620a:	e9 2b f6 ff ff       	jmp    8010583a <alltraps>

8010620f <vector138>:
.globl vector138
vector138:
  pushl $0
8010620f:	6a 00                	push   $0x0
  pushl $138
80106211:	68 8a 00 00 00       	push   $0x8a
  jmp alltraps
80106216:	e9 1f f6 ff ff       	jmp    8010583a <alltraps>

8010621b <vector139>:
.globl vector139
vector139:
  pushl $0
8010621b:	6a 00                	push   $0x0
  pushl $139
8010621d:	68 8b 00 00 00       	push   $0x8b
  jmp alltraps
80106222:	e9 13 f6 ff ff       	jmp    8010583a <alltraps>

80106227 <vector140>:
.globl vector140
vector140:
  pushl $0
80106227:	6a 00                	push   $0x0
  pushl $140
80106229:	68 8c 00 00 00       	push   $0x8c
  jmp alltraps
8010622e:	e9 07 f6 ff ff       	jmp    8010583a <alltraps>

80106233 <vector141>:
.globl vector141
vector141:
  pushl $0
80106233:	6a 00                	push   $0x0
  pushl $141
80106235:	68 8d 00 00 00       	push   $0x8d
  jmp alltraps
8010623a:	e9 fb f5 ff ff       	jmp    8010583a <alltraps>

8010623f <vector142>:
.globl vector142
vector142:
  pushl $0
8010623f:	6a 00                	push   $0x0
  pushl $142
80106241:	68 8e 00 00 00       	push   $0x8e
  jmp alltraps
80106246:	e9 ef f5 ff ff       	jmp    8010583a <alltraps>

8010624b <vector143>:
.globl vector143
vector143:
  pushl $0
8010624b:	6a 00                	push   $0x0
  pushl $143
8010624d:	68 8f 00 00 00       	push   $0x8f
  jmp alltraps
80106252:	e9 e3 f5 ff ff       	jmp    8010583a <alltraps>

80106257 <vector144>:
.globl vector144
vector144:
  pushl $0
80106257:	6a 00                	push   $0x0
  pushl $144
80106259:	68 90 00 00 00       	push   $0x90
  jmp alltraps
8010625e:	e9 d7 f5 ff ff       	jmp    8010583a <alltraps>

80106263 <vector145>:
.globl vector145
vector145:
  pushl $0
80106263:	6a 00                	push   $0x0
  pushl $145
80106265:	68 91 00 00 00       	push   $0x91
  jmp alltraps
8010626a:	e9 cb f5 ff ff       	jmp    8010583a <alltraps>

8010626f <vector146>:
.globl vector146
vector146:
  pushl $0
8010626f:	6a 00                	push   $0x0
  pushl $146
80106271:	68 92 00 00 00       	push   $0x92
  jmp alltraps
80106276:	e9 bf f5 ff ff       	jmp    8010583a <alltraps>

8010627b <vector147>:
.globl vector147
vector147:
  pushl $0
8010627b:	6a 00                	push   $0x0
  pushl $147
8010627d:	68 93 00 00 00       	push   $0x93
  jmp alltraps
80106282:	e9 b3 f5 ff ff       	jmp    8010583a <alltraps>

80106287 <vector148>:
.globl vector148
vector148:
  pushl $0
80106287:	6a 00                	push   $0x0
  pushl $148
80106289:	68 94 00 00 00       	push   $0x94
  jmp alltraps
8010628e:	e9 a7 f5 ff ff       	jmp    8010583a <alltraps>

80106293 <vector149>:
.globl vector149
vector149:
  pushl $0
80106293:	6a 00                	push   $0x0
  pushl $149
80106295:	68 95 00 00 00       	push   $0x95
  jmp alltraps
8010629a:	e9 9b f5 ff ff       	jmp    8010583a <alltraps>

8010629f <vector150>:
.globl vector150
vector150:
  pushl $0
8010629f:	6a 00                	push   $0x0
  pushl $150
801062a1:	68 96 00 00 00       	push   $0x96
  jmp alltraps
801062a6:	e9 8f f5 ff ff       	jmp    8010583a <alltraps>

801062ab <vector151>:
.globl vector151
vector151:
  pushl $0
801062ab:	6a 00                	push   $0x0
  pushl $151
801062ad:	68 97 00 00 00       	push   $0x97
  jmp alltraps
801062b2:	e9 83 f5 ff ff       	jmp    8010583a <alltraps>

801062b7 <vector152>:
.globl vector152
vector152:
  pushl $0
801062b7:	6a 00                	push   $0x0
  pushl $152
801062b9:	68 98 00 00 00       	push   $0x98
  jmp alltraps
801062be:	e9 77 f5 ff ff       	jmp    8010583a <alltraps>

801062c3 <vector153>:
.globl vector153
vector153:
  pushl $0
801062c3:	6a 00                	push   $0x0
  pushl $153
801062c5:	68 99 00 00 00       	push   $0x99
  jmp alltraps
801062ca:	e9 6b f5 ff ff       	jmp    8010583a <alltraps>

801062cf <vector154>:
.globl vector154
vector154:
  pushl $0
801062cf:	6a 00                	push   $0x0
  pushl $154
801062d1:	68 9a 00 00 00       	push   $0x9a
  jmp alltraps
801062d6:	e9 5f f5 ff ff       	jmp    8010583a <alltraps>

801062db <vector155>:
.globl vector155
vector155:
  pushl $0
801062db:	6a 00                	push   $0x0
  pushl $155
801062dd:	68 9b 00 00 00       	push   $0x9b
  jmp alltraps
801062e2:	e9 53 f5 ff ff       	jmp    8010583a <alltraps>

801062e7 <vector156>:
.globl vector156
vector156:
  pushl $0
801062e7:	6a 00                	push   $0x0
  pushl $156
801062e9:	68 9c 00 00 00       	push   $0x9c
  jmp alltraps
801062ee:	e9 47 f5 ff ff       	jmp    8010583a <alltraps>

801062f3 <vector157>:
.globl vector157
vector157:
  pushl $0
801062f3:	6a 00                	push   $0x0
  pushl $157
801062f5:	68 9d 00 00 00       	push   $0x9d
  jmp alltraps
801062fa:	e9 3b f5 ff ff       	jmp    8010583a <alltraps>

801062ff <vector158>:
.globl vector158
vector158:
  pushl $0
801062ff:	6a 00                	push   $0x0
  pushl $158
80106301:	68 9e 00 00 00       	push   $0x9e
  jmp alltraps
80106306:	e9 2f f5 ff ff       	jmp    8010583a <alltraps>

8010630b <vector159>:
.globl vector159
vector159:
  pushl $0
8010630b:	6a 00                	push   $0x0
  pushl $159
8010630d:	68 9f 00 00 00       	push   $0x9f
  jmp alltraps
80106312:	e9 23 f5 ff ff       	jmp    8010583a <alltraps>

80106317 <vector160>:
.globl vector160
vector160:
  pushl $0
80106317:	6a 00                	push   $0x0
  pushl $160
80106319:	68 a0 00 00 00       	push   $0xa0
  jmp alltraps
8010631e:	e9 17 f5 ff ff       	jmp    8010583a <alltraps>

80106323 <vector161>:
.globl vector161
vector161:
  pushl $0
80106323:	6a 00                	push   $0x0
  pushl $161
80106325:	68 a1 00 00 00       	push   $0xa1
  jmp alltraps
8010632a:	e9 0b f5 ff ff       	jmp    8010583a <alltraps>

8010632f <vector162>:
.globl vector162
vector162:
  pushl $0
8010632f:	6a 00                	push   $0x0
  pushl $162
80106331:	68 a2 00 00 00       	push   $0xa2
  jmp alltraps
80106336:	e9 ff f4 ff ff       	jmp    8010583a <alltraps>

8010633b <vector163>:
.globl vector163
vector163:
  pushl $0
8010633b:	6a 00                	push   $0x0
  pushl $163
8010633d:	68 a3 00 00 00       	push   $0xa3
  jmp alltraps
80106342:	e9 f3 f4 ff ff       	jmp    8010583a <alltraps>

80106347 <vector164>:
.globl vector164
vector164:
  pushl $0
80106347:	6a 00                	push   $0x0
  pushl $164
80106349:	68 a4 00 00 00       	push   $0xa4
  jmp alltraps
8010634e:	e9 e7 f4 ff ff       	jmp    8010583a <alltraps>

80106353 <vector165>:
.globl vector165
vector165:
  pushl $0
80106353:	6a 00                	push   $0x0
  pushl $165
80106355:	68 a5 00 00 00       	push   $0xa5
  jmp alltraps
8010635a:	e9 db f4 ff ff       	jmp    8010583a <alltraps>

8010635f <vector166>:
.globl vector166
vector166:
  pushl $0
8010635f:	6a 00                	push   $0x0
  pushl $166
80106361:	68 a6 00 00 00       	push   $0xa6
  jmp alltraps
80106366:	e9 cf f4 ff ff       	jmp    8010583a <alltraps>

8010636b <vector167>:
.globl vector167
vector167:
  pushl $0
8010636b:	6a 00                	push   $0x0
  pushl $167
8010636d:	68 a7 00 00 00       	push   $0xa7
  jmp alltraps
80106372:	e9 c3 f4 ff ff       	jmp    8010583a <alltraps>

80106377 <vector168>:
.globl vector168
vector168:
  pushl $0
80106377:	6a 00                	push   $0x0
  pushl $168
80106379:	68 a8 00 00 00       	push   $0xa8
  jmp alltraps
8010637e:	e9 b7 f4 ff ff       	jmp    8010583a <alltraps>

80106383 <vector169>:
.globl vector169
vector169:
  pushl $0
80106383:	6a 00                	push   $0x0
  pushl $169
80106385:	68 a9 00 00 00       	push   $0xa9
  jmp alltraps
8010638a:	e9 ab f4 ff ff       	jmp    8010583a <alltraps>

8010638f <vector170>:
.globl vector170
vector170:
  pushl $0
8010638f:	6a 00                	push   $0x0
  pushl $170
80106391:	68 aa 00 00 00       	push   $0xaa
  jmp alltraps
80106396:	e9 9f f4 ff ff       	jmp    8010583a <alltraps>

8010639b <vector171>:
.globl vector171
vector171:
  pushl $0
8010639b:	6a 00                	push   $0x0
  pushl $171
8010639d:	68 ab 00 00 00       	push   $0xab
  jmp alltraps
801063a2:	e9 93 f4 ff ff       	jmp    8010583a <alltraps>

801063a7 <vector172>:
.globl vector172
vector172:
  pushl $0
801063a7:	6a 00                	push   $0x0
  pushl $172
801063a9:	68 ac 00 00 00       	push   $0xac
  jmp alltraps
801063ae:	e9 87 f4 ff ff       	jmp    8010583a <alltraps>

801063b3 <vector173>:
.globl vector173
vector173:
  pushl $0
801063b3:	6a 00                	push   $0x0
  pushl $173
801063b5:	68 ad 00 00 00       	push   $0xad
  jmp alltraps
801063ba:	e9 7b f4 ff ff       	jmp    8010583a <alltraps>

801063bf <vector174>:
.globl vector174
vector174:
  pushl $0
801063bf:	6a 00                	push   $0x0
  pushl $174
801063c1:	68 ae 00 00 00       	push   $0xae
  jmp alltraps
801063c6:	e9 6f f4 ff ff       	jmp    8010583a <alltraps>

801063cb <vector175>:
.globl vector175
vector175:
  pushl $0
801063cb:	6a 00                	push   $0x0
  pushl $175
801063cd:	68 af 00 00 00       	push   $0xaf
  jmp alltraps
801063d2:	e9 63 f4 ff ff       	jmp    8010583a <alltraps>

801063d7 <vector176>:
.globl vector176
vector176:
  pushl $0
801063d7:	6a 00                	push   $0x0
  pushl $176
801063d9:	68 b0 00 00 00       	push   $0xb0
  jmp alltraps
801063de:	e9 57 f4 ff ff       	jmp    8010583a <alltraps>

801063e3 <vector177>:
.globl vector177
vector177:
  pushl $0
801063e3:	6a 00                	push   $0x0
  pushl $177
801063e5:	68 b1 00 00 00       	push   $0xb1
  jmp alltraps
801063ea:	e9 4b f4 ff ff       	jmp    8010583a <alltraps>

801063ef <vector178>:
.globl vector178
vector178:
  pushl $0
801063ef:	6a 00                	push   $0x0
  pushl $178
801063f1:	68 b2 00 00 00       	push   $0xb2
  jmp alltraps
801063f6:	e9 3f f4 ff ff       	jmp    8010583a <alltraps>

801063fb <vector179>:
.globl vector179
vector179:
  pushl $0
801063fb:	6a 00                	push   $0x0
  pushl $179
801063fd:	68 b3 00 00 00       	push   $0xb3
  jmp alltraps
80106402:	e9 33 f4 ff ff       	jmp    8010583a <alltraps>

80106407 <vector180>:
.globl vector180
vector180:
  pushl $0
80106407:	6a 00                	push   $0x0
  pushl $180
80106409:	68 b4 00 00 00       	push   $0xb4
  jmp alltraps
8010640e:	e9 27 f4 ff ff       	jmp    8010583a <alltraps>

80106413 <vector181>:
.globl vector181
vector181:
  pushl $0
80106413:	6a 00                	push   $0x0
  pushl $181
80106415:	68 b5 00 00 00       	push   $0xb5
  jmp alltraps
8010641a:	e9 1b f4 ff ff       	jmp    8010583a <alltraps>

8010641f <vector182>:
.globl vector182
vector182:
  pushl $0
8010641f:	6a 00                	push   $0x0
  pushl $182
80106421:	68 b6 00 00 00       	push   $0xb6
  jmp alltraps
80106426:	e9 0f f4 ff ff       	jmp    8010583a <alltraps>

8010642b <vector183>:
.globl vector183
vector183:
  pushl $0
8010642b:	6a 00                	push   $0x0
  pushl $183
8010642d:	68 b7 00 00 00       	push   $0xb7
  jmp alltraps
80106432:	e9 03 f4 ff ff       	jmp    8010583a <alltraps>

80106437 <vector184>:
.globl vector184
vector184:
  pushl $0
80106437:	6a 00                	push   $0x0
  pushl $184
80106439:	68 b8 00 00 00       	push   $0xb8
  jmp alltraps
8010643e:	e9 f7 f3 ff ff       	jmp    8010583a <alltraps>

80106443 <vector185>:
.globl vector185
vector185:
  pushl $0
80106443:	6a 00                	push   $0x0
  pushl $185
80106445:	68 b9 00 00 00       	push   $0xb9
  jmp alltraps
8010644a:	e9 eb f3 ff ff       	jmp    8010583a <alltraps>

8010644f <vector186>:
.globl vector186
vector186:
  pushl $0
8010644f:	6a 00                	push   $0x0
  pushl $186
80106451:	68 ba 00 00 00       	push   $0xba
  jmp alltraps
80106456:	e9 df f3 ff ff       	jmp    8010583a <alltraps>

8010645b <vector187>:
.globl vector187
vector187:
  pushl $0
8010645b:	6a 00                	push   $0x0
  pushl $187
8010645d:	68 bb 00 00 00       	push   $0xbb
  jmp alltraps
80106462:	e9 d3 f3 ff ff       	jmp    8010583a <alltraps>

80106467 <vector188>:
.globl vector188
vector188:
  pushl $0
80106467:	6a 00                	push   $0x0
  pushl $188
80106469:	68 bc 00 00 00       	push   $0xbc
  jmp alltraps
8010646e:	e9 c7 f3 ff ff       	jmp    8010583a <alltraps>

80106473 <vector189>:
.globl vector189
vector189:
  pushl $0
80106473:	6a 00                	push   $0x0
  pushl $189
80106475:	68 bd 00 00 00       	push   $0xbd
  jmp alltraps
8010647a:	e9 bb f3 ff ff       	jmp    8010583a <alltraps>

8010647f <vector190>:
.globl vector190
vector190:
  pushl $0
8010647f:	6a 00                	push   $0x0
  pushl $190
80106481:	68 be 00 00 00       	push   $0xbe
  jmp alltraps
80106486:	e9 af f3 ff ff       	jmp    8010583a <alltraps>

8010648b <vector191>:
.globl vector191
vector191:
  pushl $0
8010648b:	6a 00                	push   $0x0
  pushl $191
8010648d:	68 bf 00 00 00       	push   $0xbf
  jmp alltraps
80106492:	e9 a3 f3 ff ff       	jmp    8010583a <alltraps>

80106497 <vector192>:
.globl vector192
vector192:
  pushl $0
80106497:	6a 00                	push   $0x0
  pushl $192
80106499:	68 c0 00 00 00       	push   $0xc0
  jmp alltraps
8010649e:	e9 97 f3 ff ff       	jmp    8010583a <alltraps>

801064a3 <vector193>:
.globl vector193
vector193:
  pushl $0
801064a3:	6a 00                	push   $0x0
  pushl $193
801064a5:	68 c1 00 00 00       	push   $0xc1
  jmp alltraps
801064aa:	e9 8b f3 ff ff       	jmp    8010583a <alltraps>

801064af <vector194>:
.globl vector194
vector194:
  pushl $0
801064af:	6a 00                	push   $0x0
  pushl $194
801064b1:	68 c2 00 00 00       	push   $0xc2
  jmp alltraps
801064b6:	e9 7f f3 ff ff       	jmp    8010583a <alltraps>

801064bb <vector195>:
.globl vector195
vector195:
  pushl $0
801064bb:	6a 00                	push   $0x0
  pushl $195
801064bd:	68 c3 00 00 00       	push   $0xc3
  jmp alltraps
801064c2:	e9 73 f3 ff ff       	jmp    8010583a <alltraps>

801064c7 <vector196>:
.globl vector196
vector196:
  pushl $0
801064c7:	6a 00                	push   $0x0
  pushl $196
801064c9:	68 c4 00 00 00       	push   $0xc4
  jmp alltraps
801064ce:	e9 67 f3 ff ff       	jmp    8010583a <alltraps>

801064d3 <vector197>:
.globl vector197
vector197:
  pushl $0
801064d3:	6a 00                	push   $0x0
  pushl $197
801064d5:	68 c5 00 00 00       	push   $0xc5
  jmp alltraps
801064da:	e9 5b f3 ff ff       	jmp    8010583a <alltraps>

801064df <vector198>:
.globl vector198
vector198:
  pushl $0
801064df:	6a 00                	push   $0x0
  pushl $198
801064e1:	68 c6 00 00 00       	push   $0xc6
  jmp alltraps
801064e6:	e9 4f f3 ff ff       	jmp    8010583a <alltraps>

801064eb <vector199>:
.globl vector199
vector199:
  pushl $0
801064eb:	6a 00                	push   $0x0
  pushl $199
801064ed:	68 c7 00 00 00       	push   $0xc7
  jmp alltraps
801064f2:	e9 43 f3 ff ff       	jmp    8010583a <alltraps>

801064f7 <vector200>:
.globl vector200
vector200:
  pushl $0
801064f7:	6a 00                	push   $0x0
  pushl $200
801064f9:	68 c8 00 00 00       	push   $0xc8
  jmp alltraps
801064fe:	e9 37 f3 ff ff       	jmp    8010583a <alltraps>

80106503 <vector201>:
.globl vector201
vector201:
  pushl $0
80106503:	6a 00                	push   $0x0
  pushl $201
80106505:	68 c9 00 00 00       	push   $0xc9
  jmp alltraps
8010650a:	e9 2b f3 ff ff       	jmp    8010583a <alltraps>

8010650f <vector202>:
.globl vector202
vector202:
  pushl $0
8010650f:	6a 00                	push   $0x0
  pushl $202
80106511:	68 ca 00 00 00       	push   $0xca
  jmp alltraps
80106516:	e9 1f f3 ff ff       	jmp    8010583a <alltraps>

8010651b <vector203>:
.globl vector203
vector203:
  pushl $0
8010651b:	6a 00                	push   $0x0
  pushl $203
8010651d:	68 cb 00 00 00       	push   $0xcb
  jmp alltraps
80106522:	e9 13 f3 ff ff       	jmp    8010583a <alltraps>

80106527 <vector204>:
.globl vector204
vector204:
  pushl $0
80106527:	6a 00                	push   $0x0
  pushl $204
80106529:	68 cc 00 00 00       	push   $0xcc
  jmp alltraps
8010652e:	e9 07 f3 ff ff       	jmp    8010583a <alltraps>

80106533 <vector205>:
.globl vector205
vector205:
  pushl $0
80106533:	6a 00                	push   $0x0
  pushl $205
80106535:	68 cd 00 00 00       	push   $0xcd
  jmp alltraps
8010653a:	e9 fb f2 ff ff       	jmp    8010583a <alltraps>

8010653f <vector206>:
.globl vector206
vector206:
  pushl $0
8010653f:	6a 00                	push   $0x0
  pushl $206
80106541:	68 ce 00 00 00       	push   $0xce
  jmp alltraps
80106546:	e9 ef f2 ff ff       	jmp    8010583a <alltraps>

8010654b <vector207>:
.globl vector207
vector207:
  pushl $0
8010654b:	6a 00                	push   $0x0
  pushl $207
8010654d:	68 cf 00 00 00       	push   $0xcf
  jmp alltraps
80106552:	e9 e3 f2 ff ff       	jmp    8010583a <alltraps>

80106557 <vector208>:
.globl vector208
vector208:
  pushl $0
80106557:	6a 00                	push   $0x0
  pushl $208
80106559:	68 d0 00 00 00       	push   $0xd0
  jmp alltraps
8010655e:	e9 d7 f2 ff ff       	jmp    8010583a <alltraps>

80106563 <vector209>:
.globl vector209
vector209:
  pushl $0
80106563:	6a 00                	push   $0x0
  pushl $209
80106565:	68 d1 00 00 00       	push   $0xd1
  jmp alltraps
8010656a:	e9 cb f2 ff ff       	jmp    8010583a <alltraps>

8010656f <vector210>:
.globl vector210
vector210:
  pushl $0
8010656f:	6a 00                	push   $0x0
  pushl $210
80106571:	68 d2 00 00 00       	push   $0xd2
  jmp alltraps
80106576:	e9 bf f2 ff ff       	jmp    8010583a <alltraps>

8010657b <vector211>:
.globl vector211
vector211:
  pushl $0
8010657b:	6a 00                	push   $0x0
  pushl $211
8010657d:	68 d3 00 00 00       	push   $0xd3
  jmp alltraps
80106582:	e9 b3 f2 ff ff       	jmp    8010583a <alltraps>

80106587 <vector212>:
.globl vector212
vector212:
  pushl $0
80106587:	6a 00                	push   $0x0
  pushl $212
80106589:	68 d4 00 00 00       	push   $0xd4
  jmp alltraps
8010658e:	e9 a7 f2 ff ff       	jmp    8010583a <alltraps>

80106593 <vector213>:
.globl vector213
vector213:
  pushl $0
80106593:	6a 00                	push   $0x0
  pushl $213
80106595:	68 d5 00 00 00       	push   $0xd5
  jmp alltraps
8010659a:	e9 9b f2 ff ff       	jmp    8010583a <alltraps>

8010659f <vector214>:
.globl vector214
vector214:
  pushl $0
8010659f:	6a 00                	push   $0x0
  pushl $214
801065a1:	68 d6 00 00 00       	push   $0xd6
  jmp alltraps
801065a6:	e9 8f f2 ff ff       	jmp    8010583a <alltraps>

801065ab <vector215>:
.globl vector215
vector215:
  pushl $0
801065ab:	6a 00                	push   $0x0
  pushl $215
801065ad:	68 d7 00 00 00       	push   $0xd7
  jmp alltraps
801065b2:	e9 83 f2 ff ff       	jmp    8010583a <alltraps>

801065b7 <vector216>:
.globl vector216
vector216:
  pushl $0
801065b7:	6a 00                	push   $0x0
  pushl $216
801065b9:	68 d8 00 00 00       	push   $0xd8
  jmp alltraps
801065be:	e9 77 f2 ff ff       	jmp    8010583a <alltraps>

801065c3 <vector217>:
.globl vector217
vector217:
  pushl $0
801065c3:	6a 00                	push   $0x0
  pushl $217
801065c5:	68 d9 00 00 00       	push   $0xd9
  jmp alltraps
801065ca:	e9 6b f2 ff ff       	jmp    8010583a <alltraps>

801065cf <vector218>:
.globl vector218
vector218:
  pushl $0
801065cf:	6a 00                	push   $0x0
  pushl $218
801065d1:	68 da 00 00 00       	push   $0xda
  jmp alltraps
801065d6:	e9 5f f2 ff ff       	jmp    8010583a <alltraps>

801065db <vector219>:
.globl vector219
vector219:
  pushl $0
801065db:	6a 00                	push   $0x0
  pushl $219
801065dd:	68 db 00 00 00       	push   $0xdb
  jmp alltraps
801065e2:	e9 53 f2 ff ff       	jmp    8010583a <alltraps>

801065e7 <vector220>:
.globl vector220
vector220:
  pushl $0
801065e7:	6a 00                	push   $0x0
  pushl $220
801065e9:	68 dc 00 00 00       	push   $0xdc
  jmp alltraps
801065ee:	e9 47 f2 ff ff       	jmp    8010583a <alltraps>

801065f3 <vector221>:
.globl vector221
vector221:
  pushl $0
801065f3:	6a 00                	push   $0x0
  pushl $221
801065f5:	68 dd 00 00 00       	push   $0xdd
  jmp alltraps
801065fa:	e9 3b f2 ff ff       	jmp    8010583a <alltraps>

801065ff <vector222>:
.globl vector222
vector222:
  pushl $0
801065ff:	6a 00                	push   $0x0
  pushl $222
80106601:	68 de 00 00 00       	push   $0xde
  jmp alltraps
80106606:	e9 2f f2 ff ff       	jmp    8010583a <alltraps>

8010660b <vector223>:
.globl vector223
vector223:
  pushl $0
8010660b:	6a 00                	push   $0x0
  pushl $223
8010660d:	68 df 00 00 00       	push   $0xdf
  jmp alltraps
80106612:	e9 23 f2 ff ff       	jmp    8010583a <alltraps>

80106617 <vector224>:
.globl vector224
vector224:
  pushl $0
80106617:	6a 00                	push   $0x0
  pushl $224
80106619:	68 e0 00 00 00       	push   $0xe0
  jmp alltraps
8010661e:	e9 17 f2 ff ff       	jmp    8010583a <alltraps>

80106623 <vector225>:
.globl vector225
vector225:
  pushl $0
80106623:	6a 00                	push   $0x0
  pushl $225
80106625:	68 e1 00 00 00       	push   $0xe1
  jmp alltraps
8010662a:	e9 0b f2 ff ff       	jmp    8010583a <alltraps>

8010662f <vector226>:
.globl vector226
vector226:
  pushl $0
8010662f:	6a 00                	push   $0x0
  pushl $226
80106631:	68 e2 00 00 00       	push   $0xe2
  jmp alltraps
80106636:	e9 ff f1 ff ff       	jmp    8010583a <alltraps>

8010663b <vector227>:
.globl vector227
vector227:
  pushl $0
8010663b:	6a 00                	push   $0x0
  pushl $227
8010663d:	68 e3 00 00 00       	push   $0xe3
  jmp alltraps
80106642:	e9 f3 f1 ff ff       	jmp    8010583a <alltraps>

80106647 <vector228>:
.globl vector228
vector228:
  pushl $0
80106647:	6a 00                	push   $0x0
  pushl $228
80106649:	68 e4 00 00 00       	push   $0xe4
  jmp alltraps
8010664e:	e9 e7 f1 ff ff       	jmp    8010583a <alltraps>

80106653 <vector229>:
.globl vector229
vector229:
  pushl $0
80106653:	6a 00                	push   $0x0
  pushl $229
80106655:	68 e5 00 00 00       	push   $0xe5
  jmp alltraps
8010665a:	e9 db f1 ff ff       	jmp    8010583a <alltraps>

8010665f <vector230>:
.globl vector230
vector230:
  pushl $0
8010665f:	6a 00                	push   $0x0
  pushl $230
80106661:	68 e6 00 00 00       	push   $0xe6
  jmp alltraps
80106666:	e9 cf f1 ff ff       	jmp    8010583a <alltraps>

8010666b <vector231>:
.globl vector231
vector231:
  pushl $0
8010666b:	6a 00                	push   $0x0
  pushl $231
8010666d:	68 e7 00 00 00       	push   $0xe7
  jmp alltraps
80106672:	e9 c3 f1 ff ff       	jmp    8010583a <alltraps>

80106677 <vector232>:
.globl vector232
vector232:
  pushl $0
80106677:	6a 00                	push   $0x0
  pushl $232
80106679:	68 e8 00 00 00       	push   $0xe8
  jmp alltraps
8010667e:	e9 b7 f1 ff ff       	jmp    8010583a <alltraps>

80106683 <vector233>:
.globl vector233
vector233:
  pushl $0
80106683:	6a 00                	push   $0x0
  pushl $233
80106685:	68 e9 00 00 00       	push   $0xe9
  jmp alltraps
8010668a:	e9 ab f1 ff ff       	jmp    8010583a <alltraps>

8010668f <vector234>:
.globl vector234
vector234:
  pushl $0
8010668f:	6a 00                	push   $0x0
  pushl $234
80106691:	68 ea 00 00 00       	push   $0xea
  jmp alltraps
80106696:	e9 9f f1 ff ff       	jmp    8010583a <alltraps>

8010669b <vector235>:
.globl vector235
vector235:
  pushl $0
8010669b:	6a 00                	push   $0x0
  pushl $235
8010669d:	68 eb 00 00 00       	push   $0xeb
  jmp alltraps
801066a2:	e9 93 f1 ff ff       	jmp    8010583a <alltraps>

801066a7 <vector236>:
.globl vector236
vector236:
  pushl $0
801066a7:	6a 00                	push   $0x0
  pushl $236
801066a9:	68 ec 00 00 00       	push   $0xec
  jmp alltraps
801066ae:	e9 87 f1 ff ff       	jmp    8010583a <alltraps>

801066b3 <vector237>:
.globl vector237
vector237:
  pushl $0
801066b3:	6a 00                	push   $0x0
  pushl $237
801066b5:	68 ed 00 00 00       	push   $0xed
  jmp alltraps
801066ba:	e9 7b f1 ff ff       	jmp    8010583a <alltraps>

801066bf <vector238>:
.globl vector238
vector238:
  pushl $0
801066bf:	6a 00                	push   $0x0
  pushl $238
801066c1:	68 ee 00 00 00       	push   $0xee
  jmp alltraps
801066c6:	e9 6f f1 ff ff       	jmp    8010583a <alltraps>

801066cb <vector239>:
.globl vector239
vector239:
  pushl $0
801066cb:	6a 00                	push   $0x0
  pushl $239
801066cd:	68 ef 00 00 00       	push   $0xef
  jmp alltraps
801066d2:	e9 63 f1 ff ff       	jmp    8010583a <alltraps>

801066d7 <vector240>:
.globl vector240
vector240:
  pushl $0
801066d7:	6a 00                	push   $0x0
  pushl $240
801066d9:	68 f0 00 00 00       	push   $0xf0
  jmp alltraps
801066de:	e9 57 f1 ff ff       	jmp    8010583a <alltraps>

801066e3 <vector241>:
.globl vector241
vector241:
  pushl $0
801066e3:	6a 00                	push   $0x0
  pushl $241
801066e5:	68 f1 00 00 00       	push   $0xf1
  jmp alltraps
801066ea:	e9 4b f1 ff ff       	jmp    8010583a <alltraps>

801066ef <vector242>:
.globl vector242
vector242:
  pushl $0
801066ef:	6a 00                	push   $0x0
  pushl $242
801066f1:	68 f2 00 00 00       	push   $0xf2
  jmp alltraps
801066f6:	e9 3f f1 ff ff       	jmp    8010583a <alltraps>

801066fb <vector243>:
.globl vector243
vector243:
  pushl $0
801066fb:	6a 00                	push   $0x0
  pushl $243
801066fd:	68 f3 00 00 00       	push   $0xf3
  jmp alltraps
80106702:	e9 33 f1 ff ff       	jmp    8010583a <alltraps>

80106707 <vector244>:
.globl vector244
vector244:
  pushl $0
80106707:	6a 00                	push   $0x0
  pushl $244
80106709:	68 f4 00 00 00       	push   $0xf4
  jmp alltraps
8010670e:	e9 27 f1 ff ff       	jmp    8010583a <alltraps>

80106713 <vector245>:
.globl vector245
vector245:
  pushl $0
80106713:	6a 00                	push   $0x0
  pushl $245
80106715:	68 f5 00 00 00       	push   $0xf5
  jmp alltraps
8010671a:	e9 1b f1 ff ff       	jmp    8010583a <alltraps>

8010671f <vector246>:
.globl vector246
vector246:
  pushl $0
8010671f:	6a 00                	push   $0x0
  pushl $246
80106721:	68 f6 00 00 00       	push   $0xf6
  jmp alltraps
80106726:	e9 0f f1 ff ff       	jmp    8010583a <alltraps>

8010672b <vector247>:
.globl vector247
vector247:
  pushl $0
8010672b:	6a 00                	push   $0x0
  pushl $247
8010672d:	68 f7 00 00 00       	push   $0xf7
  jmp alltraps
80106732:	e9 03 f1 ff ff       	jmp    8010583a <alltraps>

80106737 <vector248>:
.globl vector248
vector248:
  pushl $0
80106737:	6a 00                	push   $0x0
  pushl $248
80106739:	68 f8 00 00 00       	push   $0xf8
  jmp alltraps
8010673e:	e9 f7 f0 ff ff       	jmp    8010583a <alltraps>

80106743 <vector249>:
.globl vector249
vector249:
  pushl $0
80106743:	6a 00                	push   $0x0
  pushl $249
80106745:	68 f9 00 00 00       	push   $0xf9
  jmp alltraps
8010674a:	e9 eb f0 ff ff       	jmp    8010583a <alltraps>

8010674f <vector250>:
.globl vector250
vector250:
  pushl $0
8010674f:	6a 00                	push   $0x0
  pushl $250
80106751:	68 fa 00 00 00       	push   $0xfa
  jmp alltraps
80106756:	e9 df f0 ff ff       	jmp    8010583a <alltraps>

8010675b <vector251>:
.globl vector251
vector251:
  pushl $0
8010675b:	6a 00                	push   $0x0
  pushl $251
8010675d:	68 fb 00 00 00       	push   $0xfb
  jmp alltraps
80106762:	e9 d3 f0 ff ff       	jmp    8010583a <alltraps>

80106767 <vector252>:
.globl vector252
vector252:
  pushl $0
80106767:	6a 00                	push   $0x0
  pushl $252
80106769:	68 fc 00 00 00       	push   $0xfc
  jmp alltraps
8010676e:	e9 c7 f0 ff ff       	jmp    8010583a <alltraps>

80106773 <vector253>:
.globl vector253
vector253:
  pushl $0
80106773:	6a 00                	push   $0x0
  pushl $253
80106775:	68 fd 00 00 00       	push   $0xfd
  jmp alltraps
8010677a:	e9 bb f0 ff ff       	jmp    8010583a <alltraps>

8010677f <vector254>:
.globl vector254
vector254:
  pushl $0
8010677f:	6a 00                	push   $0x0
  pushl $254
80106781:	68 fe 00 00 00       	push   $0xfe
  jmp alltraps
80106786:	e9 af f0 ff ff       	jmp    8010583a <alltraps>

8010678b <vector255>:
.globl vector255
vector255:
  pushl $0
8010678b:	6a 00                	push   $0x0
  pushl $255
8010678d:	68 ff 00 00 00       	push   $0xff
  jmp alltraps
80106792:	e9 a3 f0 ff ff       	jmp    8010583a <alltraps>
80106797:	66 90                	xchg   %ax,%ax
80106799:	66 90                	xchg   %ax,%ax
8010679b:	66 90                	xchg   %ax,%ax
8010679d:	66 90                	xchg   %ax,%ax
8010679f:	90                   	nop

801067a0 <deallocuvm.part.0>:
// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
int
deallocuvm(pde_t *pgdir, uint oldsz, uint newsz)
801067a0:	55                   	push   %ebp
801067a1:	89 e5                	mov    %esp,%ebp
801067a3:	57                   	push   %edi
801067a4:	56                   	push   %esi
801067a5:	53                   	push   %ebx
  uint a, pa;

  if(newsz >= oldsz)
    return oldsz;

  a = PGROUNDUP(newsz);
801067a6:	8d 99 ff 0f 00 00    	lea    0xfff(%ecx),%ebx
801067ac:	81 e3 00 f0 ff ff    	and    $0xfffff000,%ebx
deallocuvm(pde_t *pgdir, uint oldsz, uint newsz)
801067b2:	83 ec 1c             	sub    $0x1c,%esp
  for(; a  < oldsz; a += PGSIZE){
801067b5:	39 d3                	cmp    %edx,%ebx
801067b7:	73 56                	jae    8010680f <deallocuvm.part.0+0x6f>
801067b9:	89 4d e0             	mov    %ecx,-0x20(%ebp)
801067bc:	89 c6                	mov    %eax,%esi
801067be:	89 d7                	mov    %edx,%edi
801067c0:	eb 12                	jmp    801067d4 <deallocuvm.part.0+0x34>
801067c2:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    pte = walkpgdir(pgdir, (char*)a, 0);
    if(!pte)
      a = PGADDR(PDX(a) + 1, 0, 0) - PGSIZE;
801067c8:	83 c2 01             	add    $0x1,%edx
801067cb:	89 d3                	mov    %edx,%ebx
801067cd:	c1 e3 16             	shl    $0x16,%ebx
  for(; a  < oldsz; a += PGSIZE){
801067d0:	39 fb                	cmp    %edi,%ebx
801067d2:	73 38                	jae    8010680c <deallocuvm.part.0+0x6c>
  pde = &pgdir[PDX(va)];
801067d4:	89 da                	mov    %ebx,%edx
801067d6:	c1 ea 16             	shr    $0x16,%edx
  if(*pde & PTE_P){
801067d9:	8b 04 96             	mov    (%esi,%edx,4),%eax
801067dc:	a8 01                	test   $0x1,%al
801067de:	74 e8                	je     801067c8 <deallocuvm.part.0+0x28>
  return &pgtab[PTX(va)];
801067e0:	89 d9                	mov    %ebx,%ecx
    pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
801067e2:	25 00 f0 ff ff       	and    $0xfffff000,%eax
  return &pgtab[PTX(va)];
801067e7:	c1 e9 0a             	shr    $0xa,%ecx
801067ea:	81 e1 fc 0f 00 00    	and    $0xffc,%ecx
801067f0:	8d 84 08 00 00 00 80 	lea    -0x80000000(%eax,%ecx,1),%eax
    if(!pte)
801067f7:	85 c0                	test   %eax,%eax
801067f9:	74 cd                	je     801067c8 <deallocuvm.part.0+0x28>
    else if((*pte & PTE_P) != 0){
801067fb:	8b 10                	mov    (%eax),%edx
801067fd:	f6 c2 01             	test   $0x1,%dl
80106800:	75 1e                	jne    80106820 <deallocuvm.part.0+0x80>
  for(; a  < oldsz; a += PGSIZE){
80106802:	81 c3 00 10 00 00    	add    $0x1000,%ebx
80106808:	39 fb                	cmp    %edi,%ebx
8010680a:	72 c8                	jb     801067d4 <deallocuvm.part.0+0x34>
8010680c:	8b 4d e0             	mov    -0x20(%ebp),%ecx
      kfree(v);
      *pte = 0;
    }
  }
  return newsz;
}
8010680f:	8d 65 f4             	lea    -0xc(%ebp),%esp
80106812:	89 c8                	mov    %ecx,%eax
80106814:	5b                   	pop    %ebx
80106815:	5e                   	pop    %esi
80106816:	5f                   	pop    %edi
80106817:	5d                   	pop    %ebp
80106818:	c3                   	ret
80106819:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
      if(pa == 0)
80106820:	81 e2 00 f0 ff ff    	and    $0xfffff000,%edx
80106826:	74 26                	je     8010684e <deallocuvm.part.0+0xae>
      kfree(v);
80106828:	83 ec 0c             	sub    $0xc,%esp
      char *v = P2V(pa);
8010682b:	81 c2 00 00 00 80    	add    $0x80000000,%edx
80106831:	89 45 e4             	mov    %eax,-0x1c(%ebp)
  for(; a  < oldsz; a += PGSIZE){
80106834:	81 c3 00 10 00 00    	add    $0x1000,%ebx
      kfree(v);
8010683a:	52                   	push   %edx
8010683b:	e8 60 bc ff ff       	call   801024a0 <kfree>
      *pte = 0;
80106840:	8b 45 e4             	mov    -0x1c(%ebp),%eax
  for(; a  < oldsz; a += PGSIZE){
80106843:	83 c4 10             	add    $0x10,%esp
      *pte = 0;
80106846:	c7 00 00 00 00 00    	movl   $0x0,(%eax)
8010684c:	eb 82                	jmp    801067d0 <deallocuvm.part.0+0x30>
        panic("kfree");
8010684e:	83 ec 0c             	sub    $0xc,%esp
80106851:	68 2c 73 10 80       	push   $0x8010732c
80106856:	e8 25 9b ff ff       	call   80100380 <panic>
8010685b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi

80106860 <mappages>:
{
80106860:	55                   	push   %ebp
80106861:	89 e5                	mov    %esp,%ebp
80106863:	57                   	push   %edi
80106864:	56                   	push   %esi
80106865:	53                   	push   %ebx
  a = (char*)PGROUNDDOWN((uint)va);
80106866:	89 d3                	mov    %edx,%ebx
80106868:	81 e3 00 f0 ff ff    	and    $0xfffff000,%ebx
{
8010686e:	83 ec 1c             	sub    $0x1c,%esp
80106871:	89 45 e0             	mov    %eax,-0x20(%ebp)
  last = (char*)PGROUNDDOWN(((uint)va) + size - 1);
80106874:	8d 44 0a ff          	lea    -0x1(%edx,%ecx,1),%eax
80106878:	25 00 f0 ff ff       	and    $0xfffff000,%eax
8010687d:	89 45 dc             	mov    %eax,-0x24(%ebp)
80106880:	8b 45 08             	mov    0x8(%ebp),%eax
80106883:	29 d8                	sub    %ebx,%eax
80106885:	89 45 e4             	mov    %eax,-0x1c(%ebp)
80106888:	eb 3f                	jmp    801068c9 <mappages+0x69>
8010688a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
  return &pgtab[PTX(va)];
80106890:	89 da                	mov    %ebx,%edx
    pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
80106892:	25 00 f0 ff ff       	and    $0xfffff000,%eax
  return &pgtab[PTX(va)];
80106897:	c1 ea 0a             	shr    $0xa,%edx
8010689a:	81 e2 fc 0f 00 00    	and    $0xffc,%edx
801068a0:	8d 84 10 00 00 00 80 	lea    -0x80000000(%eax,%edx,1),%eax
    if((pte = walkpgdir(pgdir, a, 1)) == 0)
801068a7:	85 c0                	test   %eax,%eax
801068a9:	74 75                	je     80106920 <mappages+0xc0>
    if(*pte & PTE_P)
801068ab:	f6 00 01             	testb  $0x1,(%eax)
801068ae:	0f 85 86 00 00 00    	jne    8010693a <mappages+0xda>
    *pte = pa | perm | PTE_P;
801068b4:	0b 75 0c             	or     0xc(%ebp),%esi
801068b7:	83 ce 01             	or     $0x1,%esi
801068ba:	89 30                	mov    %esi,(%eax)
    if(a == last)
801068bc:	8b 45 dc             	mov    -0x24(%ebp),%eax
801068bf:	39 c3                	cmp    %eax,%ebx
801068c1:	74 6d                	je     80106930 <mappages+0xd0>
    a += PGSIZE;
801068c3:	81 c3 00 10 00 00    	add    $0x1000,%ebx
  for(;;){
801068c9:	8b 45 e4             	mov    -0x1c(%ebp),%eax
  pde = &pgdir[PDX(va)];
801068cc:	8b 4d e0             	mov    -0x20(%ebp),%ecx
801068cf:	8d 34 03             	lea    (%ebx,%eax,1),%esi
801068d2:	89 d8                	mov    %ebx,%eax
801068d4:	c1 e8 16             	shr    $0x16,%eax
801068d7:	8d 3c 81             	lea    (%ecx,%eax,4),%edi
  if(*pde & PTE_P){
801068da:	8b 07                	mov    (%edi),%eax
801068dc:	a8 01                	test   $0x1,%al
801068de:	75 b0                	jne    80106890 <mappages+0x30>
    if(!alloc || (pgtab = (pte_t*)kalloc()) == 0)
801068e0:	e8 7b bd ff ff       	call   80102660 <kalloc>
801068e5:	85 c0                	test   %eax,%eax
801068e7:	74 37                	je     80106920 <mappages+0xc0>
    memset(pgtab, 0, PGSIZE);
801068e9:	83 ec 04             	sub    $0x4,%esp
801068ec:	68 00 10 00 00       	push   $0x1000
801068f1:	6a 00                	push   $0x0
801068f3:	50                   	push   %eax
801068f4:	89 45 d8             	mov    %eax,-0x28(%ebp)
801068f7:	e8 a4 dd ff ff       	call   801046a0 <memset>
    *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
801068fc:	8b 55 d8             	mov    -0x28(%ebp),%edx
  return &pgtab[PTX(va)];
801068ff:	83 c4 10             	add    $0x10,%esp
    *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
80106902:	8d 82 00 00 00 80    	lea    -0x80000000(%edx),%eax
80106908:	83 c8 07             	or     $0x7,%eax
8010690b:	89 07                	mov    %eax,(%edi)
  return &pgtab[PTX(va)];
8010690d:	89 d8                	mov    %ebx,%eax
8010690f:	c1 e8 0a             	shr    $0xa,%eax
80106912:	25 fc 0f 00 00       	and    $0xffc,%eax
80106917:	01 d0                	add    %edx,%eax
80106919:	eb 90                	jmp    801068ab <mappages+0x4b>
8010691b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
}
80106920:	8d 65 f4             	lea    -0xc(%ebp),%esp
      return -1;
80106923:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
}
80106928:	5b                   	pop    %ebx
80106929:	5e                   	pop    %esi
8010692a:	5f                   	pop    %edi
8010692b:	5d                   	pop    %ebp
8010692c:	c3                   	ret
8010692d:	8d 76 00             	lea    0x0(%esi),%esi
80106930:	8d 65 f4             	lea    -0xc(%ebp),%esp
  return 0;
80106933:	31 c0                	xor    %eax,%eax
}
80106935:	5b                   	pop    %ebx
80106936:	5e                   	pop    %esi
80106937:	5f                   	pop    %edi
80106938:	5d                   	pop    %ebp
80106939:	c3                   	ret
      panic("remap");
8010693a:	83 ec 0c             	sub    $0xc,%esp
8010693d:	68 60 75 10 80       	push   $0x80107560
80106942:	e8 39 9a ff ff       	call   80100380 <panic>
80106947:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
8010694e:	00 
8010694f:	90                   	nop

80106950 <seginit>:
{
80106950:	55                   	push   %ebp
80106951:	89 e5                	mov    %esp,%ebp
80106953:	83 ec 18             	sub    $0x18,%esp
  c = &cpus[cpuid()];
80106956:	e8 e5 cf ff ff       	call   80103940 <cpuid>
  pd[0] = size-1;
8010695b:	ba 2f 00 00 00       	mov    $0x2f,%edx
  c->gdt[SEG_KCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, 0);
80106960:	69 c0 b0 00 00 00    	imul   $0xb0,%eax,%eax
80106966:	66 89 55 f2          	mov    %dx,-0xe(%ebp)
8010696a:	c7 80 18 18 11 80 ff 	movl   $0xffff,-0x7feee7e8(%eax)
80106971:	ff 00 00 
80106974:	c7 80 1c 18 11 80 00 	movl   $0xcf9a00,-0x7feee7e4(%eax)
8010697b:	9a cf 00 
  c->gdt[SEG_KDATA] = SEG(STA_W, 0, 0xffffffff, 0);
8010697e:	c7 80 20 18 11 80 ff 	movl   $0xffff,-0x7feee7e0(%eax)
80106985:	ff 00 00 
80106988:	c7 80 24 18 11 80 00 	movl   $0xcf9200,-0x7feee7dc(%eax)
8010698f:	92 cf 00 
  c->gdt[SEG_UCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, DPL_USER);
80106992:	c7 80 28 18 11 80 ff 	movl   $0xffff,-0x7feee7d8(%eax)
80106999:	ff 00 00 
8010699c:	c7 80 2c 18 11 80 00 	movl   $0xcffa00,-0x7feee7d4(%eax)
801069a3:	fa cf 00 
  c->gdt[SEG_UDATA] = SEG(STA_W, 0, 0xffffffff, DPL_USER);
801069a6:	c7 80 30 18 11 80 ff 	movl   $0xffff,-0x7feee7d0(%eax)
801069ad:	ff 00 00 
801069b0:	c7 80 34 18 11 80 00 	movl   $0xcff200,-0x7feee7cc(%eax)
801069b7:	f2 cf 00 
  lgdt(c->gdt, sizeof(c->gdt));
801069ba:	05 10 18 11 80       	add    $0x80111810,%eax
  pd[1] = (uint)p;
801069bf:	66 89 45 f4          	mov    %ax,-0xc(%ebp)
  pd[2] = (uint)p >> 16;
801069c3:	c1 e8 10             	shr    $0x10,%eax
801069c6:	66 89 45 f6          	mov    %ax,-0xa(%ebp)
  asm volatile("lgdt (%0)" : : "r" (pd));
801069ca:	8d 45 f2             	lea    -0xe(%ebp),%eax
801069cd:	0f 01 10             	lgdtl  (%eax)
}
801069d0:	c9                   	leave
801069d1:	c3                   	ret
801069d2:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
801069d9:	00 
801069da:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi

801069e0 <switchkvm>:
  lcr3(V2P(kpgdir));   // switch to the kernel page table
801069e0:	a1 c4 44 11 80       	mov    0x801144c4,%eax
801069e5:	05 00 00 00 80       	add    $0x80000000,%eax
}

static inline void
lcr3(uint val)
{
  asm volatile("movl %0,%%cr3" : : "r" (val));
801069ea:	0f 22 d8             	mov    %eax,%cr3
}
801069ed:	c3                   	ret
801069ee:	66 90                	xchg   %ax,%ax

801069f0 <switchuvm>:
{
801069f0:	55                   	push   %ebp
801069f1:	89 e5                	mov    %esp,%ebp
801069f3:	57                   	push   %edi
801069f4:	56                   	push   %esi
801069f5:	53                   	push   %ebx
801069f6:	83 ec 1c             	sub    $0x1c,%esp
801069f9:	8b 75 08             	mov    0x8(%ebp),%esi
  if(p == 0)
801069fc:	85 f6                	test   %esi,%esi
801069fe:	0f 84 cb 00 00 00    	je     80106acf <switchuvm+0xdf>
  if(p->kstack == 0)
80106a04:	8b 46 08             	mov    0x8(%esi),%eax
80106a07:	85 c0                	test   %eax,%eax
80106a09:	0f 84 da 00 00 00    	je     80106ae9 <switchuvm+0xf9>
  if(p->pgdir == 0)
80106a0f:	8b 46 04             	mov    0x4(%esi),%eax
80106a12:	85 c0                	test   %eax,%eax
80106a14:	0f 84 c2 00 00 00    	je     80106adc <switchuvm+0xec>
  pushcli();
80106a1a:	e8 31 da ff ff       	call   80104450 <pushcli>
  mycpu()->gdt[SEG_TSS] = SEG16(STS_T32A, &mycpu()->ts,
80106a1f:	e8 bc ce ff ff       	call   801038e0 <mycpu>
80106a24:	89 c3                	mov    %eax,%ebx
80106a26:	e8 b5 ce ff ff       	call   801038e0 <mycpu>
80106a2b:	89 c7                	mov    %eax,%edi
80106a2d:	e8 ae ce ff ff       	call   801038e0 <mycpu>
80106a32:	83 c7 08             	add    $0x8,%edi
80106a35:	89 45 e4             	mov    %eax,-0x1c(%ebp)
80106a38:	e8 a3 ce ff ff       	call   801038e0 <mycpu>
80106a3d:	8b 4d e4             	mov    -0x1c(%ebp),%ecx
80106a40:	ba 67 00 00 00       	mov    $0x67,%edx
80106a45:	66 89 bb 9a 00 00 00 	mov    %di,0x9a(%ebx)
80106a4c:	83 c0 08             	add    $0x8,%eax
80106a4f:	66 89 93 98 00 00 00 	mov    %dx,0x98(%ebx)
  mycpu()->ts.iomb = (ushort) 0xFFFF;
80106a56:	bf ff ff ff ff       	mov    $0xffffffff,%edi
  mycpu()->gdt[SEG_TSS] = SEG16(STS_T32A, &mycpu()->ts,
80106a5b:	83 c1 08             	add    $0x8,%ecx
80106a5e:	c1 e8 18             	shr    $0x18,%eax
80106a61:	c1 e9 10             	shr    $0x10,%ecx
80106a64:	88 83 9f 00 00 00    	mov    %al,0x9f(%ebx)
80106a6a:	88 8b 9c 00 00 00    	mov    %cl,0x9c(%ebx)
80106a70:	b9 99 40 00 00       	mov    $0x4099,%ecx
80106a75:	66 89 8b 9d 00 00 00 	mov    %cx,0x9d(%ebx)
  mycpu()->ts.ss0 = SEG_KDATA << 3;
80106a7c:	bb 10 00 00 00       	mov    $0x10,%ebx
  mycpu()->gdt[SEG_TSS].s = 0;
80106a81:	e8 5a ce ff ff       	call   801038e0 <mycpu>
80106a86:	80 a0 9d 00 00 00 ef 	andb   $0xef,0x9d(%eax)
  mycpu()->ts.ss0 = SEG_KDATA << 3;
80106a8d:	e8 4e ce ff ff       	call   801038e0 <mycpu>
80106a92:	66 89 58 10          	mov    %bx,0x10(%eax)
  mycpu()->ts.esp0 = (uint)p->kstack + KSTACKSIZE;
80106a96:	8b 5e 08             	mov    0x8(%esi),%ebx
80106a99:	81 c3 00 10 00 00    	add    $0x1000,%ebx
80106a9f:	e8 3c ce ff ff       	call   801038e0 <mycpu>
80106aa4:	89 58 0c             	mov    %ebx,0xc(%eax)
  mycpu()->ts.iomb = (ushort) 0xFFFF;
80106aa7:	e8 34 ce ff ff       	call   801038e0 <mycpu>
80106aac:	66 89 78 6e          	mov    %di,0x6e(%eax)
  asm volatile("ltr %0" : : "r" (sel));
80106ab0:	b8 28 00 00 00       	mov    $0x28,%eax
80106ab5:	0f 00 d8             	ltr    %eax
  lcr3(V2P(p->pgdir));  // switch to process's address space
80106ab8:	8b 46 04             	mov    0x4(%esi),%eax
80106abb:	05 00 00 00 80       	add    $0x80000000,%eax
  asm volatile("movl %0,%%cr3" : : "r" (val));
80106ac0:	0f 22 d8             	mov    %eax,%cr3
}
80106ac3:	8d 65 f4             	lea    -0xc(%ebp),%esp
80106ac6:	5b                   	pop    %ebx
80106ac7:	5e                   	pop    %esi
80106ac8:	5f                   	pop    %edi
80106ac9:	5d                   	pop    %ebp
  popcli();
80106aca:	e9 d1 d9 ff ff       	jmp    801044a0 <popcli>
    panic("switchuvm: no process");
80106acf:	83 ec 0c             	sub    $0xc,%esp
80106ad2:	68 66 75 10 80       	push   $0x80107566
80106ad7:	e8 a4 98 ff ff       	call   80100380 <panic>
    panic("switchuvm: no pgdir");
80106adc:	83 ec 0c             	sub    $0xc,%esp
80106adf:	68 91 75 10 80       	push   $0x80107591
80106ae4:	e8 97 98 ff ff       	call   80100380 <panic>
    panic("switchuvm: no kstack");
80106ae9:	83 ec 0c             	sub    $0xc,%esp
80106aec:	68 7c 75 10 80       	push   $0x8010757c
80106af1:	e8 8a 98 ff ff       	call   80100380 <panic>
80106af6:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80106afd:	00 
80106afe:	66 90                	xchg   %ax,%ax

80106b00 <inituvm>:
{
80106b00:	55                   	push   %ebp
80106b01:	89 e5                	mov    %esp,%ebp
80106b03:	57                   	push   %edi
80106b04:	56                   	push   %esi
80106b05:	53                   	push   %ebx
80106b06:	83 ec 1c             	sub    $0x1c,%esp
80106b09:	8b 45 08             	mov    0x8(%ebp),%eax
80106b0c:	8b 75 10             	mov    0x10(%ebp),%esi
80106b0f:	8b 7d 0c             	mov    0xc(%ebp),%edi
80106b12:	89 45 e4             	mov    %eax,-0x1c(%ebp)
  if(sz >= PGSIZE)
80106b15:	81 fe ff 0f 00 00    	cmp    $0xfff,%esi
80106b1b:	77 49                	ja     80106b66 <inituvm+0x66>
  mem = kalloc();
80106b1d:	e8 3e bb ff ff       	call   80102660 <kalloc>
  memset(mem, 0, PGSIZE);
80106b22:	83 ec 04             	sub    $0x4,%esp
80106b25:	68 00 10 00 00       	push   $0x1000
  mem = kalloc();
80106b2a:	89 c3                	mov    %eax,%ebx
  memset(mem, 0, PGSIZE);
80106b2c:	6a 00                	push   $0x0
80106b2e:	50                   	push   %eax
80106b2f:	e8 6c db ff ff       	call   801046a0 <memset>
  mappages(pgdir, 0, PGSIZE, V2P(mem), PTE_W|PTE_U);
80106b34:	58                   	pop    %eax
80106b35:	8d 83 00 00 00 80    	lea    -0x80000000(%ebx),%eax
80106b3b:	5a                   	pop    %edx
80106b3c:	6a 06                	push   $0x6
80106b3e:	b9 00 10 00 00       	mov    $0x1000,%ecx
80106b43:	31 d2                	xor    %edx,%edx
80106b45:	50                   	push   %eax
80106b46:	8b 45 e4             	mov    -0x1c(%ebp),%eax
80106b49:	e8 12 fd ff ff       	call   80106860 <mappages>
  memmove(mem, init, sz);
80106b4e:	83 c4 10             	add    $0x10,%esp
80106b51:	89 75 10             	mov    %esi,0x10(%ebp)
80106b54:	89 7d 0c             	mov    %edi,0xc(%ebp)
80106b57:	89 5d 08             	mov    %ebx,0x8(%ebp)
}
80106b5a:	8d 65 f4             	lea    -0xc(%ebp),%esp
80106b5d:	5b                   	pop    %ebx
80106b5e:	5e                   	pop    %esi
80106b5f:	5f                   	pop    %edi
80106b60:	5d                   	pop    %ebp
  memmove(mem, init, sz);
80106b61:	e9 ca db ff ff       	jmp    80104730 <memmove>
    panic("inituvm: more than a page");
80106b66:	83 ec 0c             	sub    $0xc,%esp
80106b69:	68 a5 75 10 80       	push   $0x801075a5
80106b6e:	e8 0d 98 ff ff       	call   80100380 <panic>
80106b73:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80106b7a:	00 
80106b7b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi

80106b80 <loaduvm>:
{
80106b80:	55                   	push   %ebp
80106b81:	89 e5                	mov    %esp,%ebp
80106b83:	57                   	push   %edi
80106b84:	56                   	push   %esi
80106b85:	53                   	push   %ebx
80106b86:	83 ec 0c             	sub    $0xc,%esp
  if((uint) addr % PGSIZE != 0)
80106b89:	8b 75 0c             	mov    0xc(%ebp),%esi
{
80106b8c:	8b 7d 18             	mov    0x18(%ebp),%edi
  if((uint) addr % PGSIZE != 0)
80106b8f:	81 e6 ff 0f 00 00    	and    $0xfff,%esi
80106b95:	0f 85 a2 00 00 00    	jne    80106c3d <loaduvm+0xbd>
  for(i = 0; i < sz; i += PGSIZE){
80106b9b:	85 ff                	test   %edi,%edi
80106b9d:	74 7d                	je     80106c1c <loaduvm+0x9c>
80106b9f:	90                   	nop
  pde = &pgdir[PDX(va)];
80106ba0:	8b 45 0c             	mov    0xc(%ebp),%eax
  if(*pde & PTE_P){
80106ba3:	8b 55 08             	mov    0x8(%ebp),%edx
80106ba6:	01 f0                	add    %esi,%eax
  pde = &pgdir[PDX(va)];
80106ba8:	89 c1                	mov    %eax,%ecx
80106baa:	c1 e9 16             	shr    $0x16,%ecx
  if(*pde & PTE_P){
80106bad:	8b 0c 8a             	mov    (%edx,%ecx,4),%ecx
80106bb0:	f6 c1 01             	test   $0x1,%cl
80106bb3:	75 13                	jne    80106bc8 <loaduvm+0x48>
      panic("loaduvm: address should exist");
80106bb5:	83 ec 0c             	sub    $0xc,%esp
80106bb8:	68 bf 75 10 80       	push   $0x801075bf
80106bbd:	e8 be 97 ff ff       	call   80100380 <panic>
80106bc2:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
  return &pgtab[PTX(va)];
80106bc8:	c1 e8 0a             	shr    $0xa,%eax
    pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
80106bcb:	81 e1 00 f0 ff ff    	and    $0xfffff000,%ecx
  return &pgtab[PTX(va)];
80106bd1:	25 fc 0f 00 00       	and    $0xffc,%eax
80106bd6:	8d 8c 01 00 00 00 80 	lea    -0x80000000(%ecx,%eax,1),%ecx
    if((pte = walkpgdir(pgdir, addr+i, 0)) == 0)
80106bdd:	85 c9                	test   %ecx,%ecx
80106bdf:	74 d4                	je     80106bb5 <loaduvm+0x35>
    if(sz - i < PGSIZE)
80106be1:	89 fb                	mov    %edi,%ebx
80106be3:	b8 00 10 00 00       	mov    $0x1000,%eax
80106be8:	29 f3                	sub    %esi,%ebx
80106bea:	39 c3                	cmp    %eax,%ebx
80106bec:	0f 47 d8             	cmova  %eax,%ebx
    if(readi(ip, P2V(pa), offset+i, n) != n)
80106bef:	53                   	push   %ebx
80106bf0:	8b 45 14             	mov    0x14(%ebp),%eax
80106bf3:	01 f0                	add    %esi,%eax
80106bf5:	50                   	push   %eax
    pa = PTE_ADDR(*pte);
80106bf6:	8b 01                	mov    (%ecx),%eax
80106bf8:	25 00 f0 ff ff       	and    $0xfffff000,%eax
    if(readi(ip, P2V(pa), offset+i, n) != n)
80106bfd:	05 00 00 00 80       	add    $0x80000000,%eax
80106c02:	50                   	push   %eax
80106c03:	ff 75 10             	push   0x10(%ebp)
80106c06:	e8 a5 ae ff ff       	call   80101ab0 <readi>
80106c0b:	83 c4 10             	add    $0x10,%esp
80106c0e:	39 d8                	cmp    %ebx,%eax
80106c10:	75 1e                	jne    80106c30 <loaduvm+0xb0>
  for(i = 0; i < sz; i += PGSIZE){
80106c12:	81 c6 00 10 00 00    	add    $0x1000,%esi
80106c18:	39 fe                	cmp    %edi,%esi
80106c1a:	72 84                	jb     80106ba0 <loaduvm+0x20>
}
80106c1c:	8d 65 f4             	lea    -0xc(%ebp),%esp
  return 0;
80106c1f:	31 c0                	xor    %eax,%eax
}
80106c21:	5b                   	pop    %ebx
80106c22:	5e                   	pop    %esi
80106c23:	5f                   	pop    %edi
80106c24:	5d                   	pop    %ebp
80106c25:	c3                   	ret
80106c26:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80106c2d:	00 
80106c2e:	66 90                	xchg   %ax,%ax
80106c30:	8d 65 f4             	lea    -0xc(%ebp),%esp
      return -1;
80106c33:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
}
80106c38:	5b                   	pop    %ebx
80106c39:	5e                   	pop    %esi
80106c3a:	5f                   	pop    %edi
80106c3b:	5d                   	pop    %ebp
80106c3c:	c3                   	ret
    panic("loaduvm: addr must be page aligned");
80106c3d:	83 ec 0c             	sub    $0xc,%esp
80106c40:	68 e0 77 10 80       	push   $0x801077e0
80106c45:	e8 36 97 ff ff       	call   80100380 <panic>
80106c4a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi

80106c50 <allocuvm>:
{
80106c50:	55                   	push   %ebp
80106c51:	89 e5                	mov    %esp,%ebp
80106c53:	57                   	push   %edi
80106c54:	56                   	push   %esi
80106c55:	53                   	push   %ebx
80106c56:	83 ec 1c             	sub    $0x1c,%esp
80106c59:	8b 75 10             	mov    0x10(%ebp),%esi
  if(newsz >= KERNBASE)
80106c5c:	85 f6                	test   %esi,%esi
80106c5e:	0f 88 98 00 00 00    	js     80106cfc <allocuvm+0xac>
80106c64:	89 f2                	mov    %esi,%edx
  if(newsz < oldsz)
80106c66:	3b 75 0c             	cmp    0xc(%ebp),%esi
80106c69:	0f 82 a1 00 00 00    	jb     80106d10 <allocuvm+0xc0>
  a = PGROUNDUP(oldsz);
80106c6f:	8b 45 0c             	mov    0xc(%ebp),%eax
80106c72:	05 ff 0f 00 00       	add    $0xfff,%eax
80106c77:	25 00 f0 ff ff       	and    $0xfffff000,%eax
80106c7c:	89 c7                	mov    %eax,%edi
  for(; a < newsz; a += PGSIZE){
80106c7e:	39 f0                	cmp    %esi,%eax
80106c80:	0f 83 8d 00 00 00    	jae    80106d13 <allocuvm+0xc3>
80106c86:	89 75 e4             	mov    %esi,-0x1c(%ebp)
80106c89:	eb 44                	jmp    80106ccf <allocuvm+0x7f>
80106c8b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
    memset(mem, 0, PGSIZE);
80106c90:	83 ec 04             	sub    $0x4,%esp
80106c93:	68 00 10 00 00       	push   $0x1000
80106c98:	6a 00                	push   $0x0
80106c9a:	50                   	push   %eax
80106c9b:	e8 00 da ff ff       	call   801046a0 <memset>
    if(mappages(pgdir, (char*)a, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0){
80106ca0:	58                   	pop    %eax
80106ca1:	8d 83 00 00 00 80    	lea    -0x80000000(%ebx),%eax
80106ca7:	5a                   	pop    %edx
80106ca8:	6a 06                	push   $0x6
80106caa:	b9 00 10 00 00       	mov    $0x1000,%ecx
80106caf:	89 fa                	mov    %edi,%edx
80106cb1:	50                   	push   %eax
80106cb2:	8b 45 08             	mov    0x8(%ebp),%eax
80106cb5:	e8 a6 fb ff ff       	call   80106860 <mappages>
80106cba:	83 c4 10             	add    $0x10,%esp
80106cbd:	85 c0                	test   %eax,%eax
80106cbf:	78 5f                	js     80106d20 <allocuvm+0xd0>
  for(; a < newsz; a += PGSIZE){
80106cc1:	81 c7 00 10 00 00    	add    $0x1000,%edi
80106cc7:	39 f7                	cmp    %esi,%edi
80106cc9:	0f 83 89 00 00 00    	jae    80106d58 <allocuvm+0x108>
    mem = kalloc();
80106ccf:	e8 8c b9 ff ff       	call   80102660 <kalloc>
80106cd4:	89 c3                	mov    %eax,%ebx
    if(mem == 0){
80106cd6:	85 c0                	test   %eax,%eax
80106cd8:	75 b6                	jne    80106c90 <allocuvm+0x40>
      cprintf("allocuvm out of memory\n");
80106cda:	83 ec 0c             	sub    $0xc,%esp
80106cdd:	68 dd 75 10 80       	push   $0x801075dd
80106ce2:	e8 c9 99 ff ff       	call   801006b0 <cprintf>
  if(newsz >= oldsz)
80106ce7:	83 c4 10             	add    $0x10,%esp
80106cea:	3b 75 0c             	cmp    0xc(%ebp),%esi
80106ced:	74 0d                	je     80106cfc <allocuvm+0xac>
80106cef:	8b 4d 0c             	mov    0xc(%ebp),%ecx
80106cf2:	8b 45 08             	mov    0x8(%ebp),%eax
80106cf5:	89 f2                	mov    %esi,%edx
80106cf7:	e8 a4 fa ff ff       	call   801067a0 <deallocuvm.part.0>
    return 0;
80106cfc:	31 d2                	xor    %edx,%edx
}
80106cfe:	8d 65 f4             	lea    -0xc(%ebp),%esp
80106d01:	89 d0                	mov    %edx,%eax
80106d03:	5b                   	pop    %ebx
80106d04:	5e                   	pop    %esi
80106d05:	5f                   	pop    %edi
80106d06:	5d                   	pop    %ebp
80106d07:	c3                   	ret
80106d08:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80106d0f:	00 
    return oldsz;
80106d10:	8b 55 0c             	mov    0xc(%ebp),%edx
}
80106d13:	8d 65 f4             	lea    -0xc(%ebp),%esp
80106d16:	89 d0                	mov    %edx,%eax
80106d18:	5b                   	pop    %ebx
80106d19:	5e                   	pop    %esi
80106d1a:	5f                   	pop    %edi
80106d1b:	5d                   	pop    %ebp
80106d1c:	c3                   	ret
80106d1d:	8d 76 00             	lea    0x0(%esi),%esi
      cprintf("allocuvm out of memory (2)\n");
80106d20:	83 ec 0c             	sub    $0xc,%esp
80106d23:	68 f5 75 10 80       	push   $0x801075f5
80106d28:	e8 83 99 ff ff       	call   801006b0 <cprintf>
  if(newsz >= oldsz)
80106d2d:	83 c4 10             	add    $0x10,%esp
80106d30:	3b 75 0c             	cmp    0xc(%ebp),%esi
80106d33:	74 0d                	je     80106d42 <allocuvm+0xf2>
80106d35:	8b 4d 0c             	mov    0xc(%ebp),%ecx
80106d38:	8b 45 08             	mov    0x8(%ebp),%eax
80106d3b:	89 f2                	mov    %esi,%edx
80106d3d:	e8 5e fa ff ff       	call   801067a0 <deallocuvm.part.0>
      kfree(mem);
80106d42:	83 ec 0c             	sub    $0xc,%esp
80106d45:	53                   	push   %ebx
80106d46:	e8 55 b7 ff ff       	call   801024a0 <kfree>
      return 0;
80106d4b:	83 c4 10             	add    $0x10,%esp
    return 0;
80106d4e:	31 d2                	xor    %edx,%edx
80106d50:	eb ac                	jmp    80106cfe <allocuvm+0xae>
80106d52:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
80106d58:	8b 55 e4             	mov    -0x1c(%ebp),%edx
}
80106d5b:	8d 65 f4             	lea    -0xc(%ebp),%esp
80106d5e:	5b                   	pop    %ebx
80106d5f:	5e                   	pop    %esi
80106d60:	89 d0                	mov    %edx,%eax
80106d62:	5f                   	pop    %edi
80106d63:	5d                   	pop    %ebp
80106d64:	c3                   	ret
80106d65:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80106d6c:	00 
80106d6d:	8d 76 00             	lea    0x0(%esi),%esi

80106d70 <deallocuvm>:
{
80106d70:	55                   	push   %ebp
80106d71:	89 e5                	mov    %esp,%ebp
80106d73:	8b 55 0c             	mov    0xc(%ebp),%edx
80106d76:	8b 4d 10             	mov    0x10(%ebp),%ecx
80106d79:	8b 45 08             	mov    0x8(%ebp),%eax
  if(newsz >= oldsz)
80106d7c:	39 d1                	cmp    %edx,%ecx
80106d7e:	73 10                	jae    80106d90 <deallocuvm+0x20>
}
80106d80:	5d                   	pop    %ebp
80106d81:	e9 1a fa ff ff       	jmp    801067a0 <deallocuvm.part.0>
80106d86:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80106d8d:	00 
80106d8e:	66 90                	xchg   %ax,%ax
80106d90:	89 d0                	mov    %edx,%eax
80106d92:	5d                   	pop    %ebp
80106d93:	c3                   	ret
80106d94:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80106d9b:	00 
80106d9c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi

80106da0 <freevm>:

// Free a page table and all the physical memory pages
// in the user part.
void
freevm(pde_t *pgdir)
{
80106da0:	55                   	push   %ebp
80106da1:	89 e5                	mov    %esp,%ebp
80106da3:	57                   	push   %edi
80106da4:	56                   	push   %esi
80106da5:	53                   	push   %ebx
80106da6:	83 ec 0c             	sub    $0xc,%esp
80106da9:	8b 75 08             	mov    0x8(%ebp),%esi
  uint i;

  if(pgdir == 0)
80106dac:	85 f6                	test   %esi,%esi
80106dae:	74 59                	je     80106e09 <freevm+0x69>
  if(newsz >= oldsz)
80106db0:	31 c9                	xor    %ecx,%ecx
80106db2:	ba 00 00 00 80       	mov    $0x80000000,%edx
80106db7:	89 f0                	mov    %esi,%eax
80106db9:	89 f3                	mov    %esi,%ebx
80106dbb:	e8 e0 f9 ff ff       	call   801067a0 <deallocuvm.part.0>
    panic("freevm: no pgdir");
  deallocuvm(pgdir, KERNBASE, 0);
  for(i = 0; i < NPDENTRIES; i++){
80106dc0:	8d be 00 10 00 00    	lea    0x1000(%esi),%edi
80106dc6:	eb 0f                	jmp    80106dd7 <freevm+0x37>
80106dc8:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80106dcf:	00 
80106dd0:	83 c3 04             	add    $0x4,%ebx
80106dd3:	39 fb                	cmp    %edi,%ebx
80106dd5:	74 23                	je     80106dfa <freevm+0x5a>
    if(pgdir[i] & PTE_P){
80106dd7:	8b 03                	mov    (%ebx),%eax
80106dd9:	a8 01                	test   $0x1,%al
80106ddb:	74 f3                	je     80106dd0 <freevm+0x30>
      char * v = P2V(PTE_ADDR(pgdir[i]));
80106ddd:	25 00 f0 ff ff       	and    $0xfffff000,%eax
      kfree(v);
80106de2:	83 ec 0c             	sub    $0xc,%esp
  for(i = 0; i < NPDENTRIES; i++){
80106de5:	83 c3 04             	add    $0x4,%ebx
      char * v = P2V(PTE_ADDR(pgdir[i]));
80106de8:	05 00 00 00 80       	add    $0x80000000,%eax
      kfree(v);
80106ded:	50                   	push   %eax
80106dee:	e8 ad b6 ff ff       	call   801024a0 <kfree>
80106df3:	83 c4 10             	add    $0x10,%esp
  for(i = 0; i < NPDENTRIES; i++){
80106df6:	39 fb                	cmp    %edi,%ebx
80106df8:	75 dd                	jne    80106dd7 <freevm+0x37>
    }
  }
  kfree((char*)pgdir);
80106dfa:	89 75 08             	mov    %esi,0x8(%ebp)
}
80106dfd:	8d 65 f4             	lea    -0xc(%ebp),%esp
80106e00:	5b                   	pop    %ebx
80106e01:	5e                   	pop    %esi
80106e02:	5f                   	pop    %edi
80106e03:	5d                   	pop    %ebp
  kfree((char*)pgdir);
80106e04:	e9 97 b6 ff ff       	jmp    801024a0 <kfree>
    panic("freevm: no pgdir");
80106e09:	83 ec 0c             	sub    $0xc,%esp
80106e0c:	68 11 76 10 80       	push   $0x80107611
80106e11:	e8 6a 95 ff ff       	call   80100380 <panic>
80106e16:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80106e1d:	00 
80106e1e:	66 90                	xchg   %ax,%ax

80106e20 <setupkvm>:
{
80106e20:	55                   	push   %ebp
80106e21:	89 e5                	mov    %esp,%ebp
80106e23:	56                   	push   %esi
80106e24:	53                   	push   %ebx
  if((pgdir = (pde_t*)kalloc()) == 0)
80106e25:	e8 36 b8 ff ff       	call   80102660 <kalloc>
80106e2a:	85 c0                	test   %eax,%eax
80106e2c:	74 5e                	je     80106e8c <setupkvm+0x6c>
  memset(pgdir, 0, PGSIZE);
80106e2e:	83 ec 04             	sub    $0x4,%esp
80106e31:	89 c6                	mov    %eax,%esi
  for(k = kmap; k < &kmap[NELEM(kmap)]; k++)
80106e33:	bb 20 a4 10 80       	mov    $0x8010a420,%ebx
  memset(pgdir, 0, PGSIZE);
80106e38:	68 00 10 00 00       	push   $0x1000
80106e3d:	6a 00                	push   $0x0
80106e3f:	50                   	push   %eax
80106e40:	e8 5b d8 ff ff       	call   801046a0 <memset>
80106e45:	83 c4 10             	add    $0x10,%esp
                (uint)k->phys_start, k->perm) < 0) {
80106e48:	8b 43 04             	mov    0x4(%ebx),%eax
    if(mappages(pgdir, k->virt, k->phys_end - k->phys_start,
80106e4b:	83 ec 08             	sub    $0x8,%esp
80106e4e:	8b 4b 08             	mov    0x8(%ebx),%ecx
80106e51:	8b 13                	mov    (%ebx),%edx
80106e53:	ff 73 0c             	push   0xc(%ebx)
80106e56:	50                   	push   %eax
80106e57:	29 c1                	sub    %eax,%ecx
80106e59:	89 f0                	mov    %esi,%eax
80106e5b:	e8 00 fa ff ff       	call   80106860 <mappages>
80106e60:	83 c4 10             	add    $0x10,%esp
80106e63:	85 c0                	test   %eax,%eax
80106e65:	78 19                	js     80106e80 <setupkvm+0x60>
  for(k = kmap; k < &kmap[NELEM(kmap)]; k++)
80106e67:	83 c3 10             	add    $0x10,%ebx
80106e6a:	81 fb 60 a4 10 80    	cmp    $0x8010a460,%ebx
80106e70:	75 d6                	jne    80106e48 <setupkvm+0x28>
}
80106e72:	8d 65 f8             	lea    -0x8(%ebp),%esp
80106e75:	89 f0                	mov    %esi,%eax
80106e77:	5b                   	pop    %ebx
80106e78:	5e                   	pop    %esi
80106e79:	5d                   	pop    %ebp
80106e7a:	c3                   	ret
80106e7b:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
      freevm(pgdir);
80106e80:	83 ec 0c             	sub    $0xc,%esp
80106e83:	56                   	push   %esi
80106e84:	e8 17 ff ff ff       	call   80106da0 <freevm>
      return 0;
80106e89:	83 c4 10             	add    $0x10,%esp
}
80106e8c:	8d 65 f8             	lea    -0x8(%ebp),%esp
    return 0;
80106e8f:	31 f6                	xor    %esi,%esi
}
80106e91:	89 f0                	mov    %esi,%eax
80106e93:	5b                   	pop    %ebx
80106e94:	5e                   	pop    %esi
80106e95:	5d                   	pop    %ebp
80106e96:	c3                   	ret
80106e97:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80106e9e:	00 
80106e9f:	90                   	nop

80106ea0 <kvmalloc>:
{
80106ea0:	55                   	push   %ebp
80106ea1:	89 e5                	mov    %esp,%ebp
80106ea3:	83 ec 08             	sub    $0x8,%esp
  kpgdir = setupkvm();
80106ea6:	e8 75 ff ff ff       	call   80106e20 <setupkvm>
80106eab:	a3 c4 44 11 80       	mov    %eax,0x801144c4
  lcr3(V2P(kpgdir));   // switch to the kernel page table
80106eb0:	05 00 00 00 80       	add    $0x80000000,%eax
80106eb5:	0f 22 d8             	mov    %eax,%cr3
}
80106eb8:	c9                   	leave
80106eb9:	c3                   	ret
80106eba:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi

80106ec0 <clearpteu>:

// Clear PTE_U on a page. Used to create an inaccessible
// page beneath the user stack.
void
clearpteu(pde_t *pgdir, char *uva)
{
80106ec0:	55                   	push   %ebp
80106ec1:	89 e5                	mov    %esp,%ebp
80106ec3:	83 ec 08             	sub    $0x8,%esp
80106ec6:	8b 45 0c             	mov    0xc(%ebp),%eax
  if(*pde & PTE_P){
80106ec9:	8b 55 08             	mov    0x8(%ebp),%edx
  pde = &pgdir[PDX(va)];
80106ecc:	89 c1                	mov    %eax,%ecx
80106ece:	c1 e9 16             	shr    $0x16,%ecx
  if(*pde & PTE_P){
80106ed1:	8b 14 8a             	mov    (%edx,%ecx,4),%edx
80106ed4:	f6 c2 01             	test   $0x1,%dl
80106ed7:	75 17                	jne    80106ef0 <clearpteu+0x30>
  pte_t *pte;

  pte = walkpgdir(pgdir, uva, 0);
  if(pte == 0)
    panic("clearpteu");
80106ed9:	83 ec 0c             	sub    $0xc,%esp
80106edc:	68 22 76 10 80       	push   $0x80107622
80106ee1:	e8 9a 94 ff ff       	call   80100380 <panic>
80106ee6:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80106eed:	00 
80106eee:	66 90                	xchg   %ax,%ax
  return &pgtab[PTX(va)];
80106ef0:	c1 e8 0a             	shr    $0xa,%eax
    pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
80106ef3:	81 e2 00 f0 ff ff    	and    $0xfffff000,%edx
  return &pgtab[PTX(va)];
80106ef9:	25 fc 0f 00 00       	and    $0xffc,%eax
80106efe:	8d 84 02 00 00 00 80 	lea    -0x80000000(%edx,%eax,1),%eax
  if(pte == 0)
80106f05:	85 c0                	test   %eax,%eax
80106f07:	74 d0                	je     80106ed9 <clearpteu+0x19>
  *pte &= ~PTE_U;
80106f09:	83 20 fb             	andl   $0xfffffffb,(%eax)
}
80106f0c:	c9                   	leave
80106f0d:	c3                   	ret
80106f0e:	66 90                	xchg   %ax,%ax

80106f10 <copyuvm>:

// Given a parent process's page table, create a copy
// of it for a child.
pde_t*
copyuvm(pde_t *pgdir, uint sz)
{
80106f10:	55                   	push   %ebp
80106f11:	89 e5                	mov    %esp,%ebp
80106f13:	57                   	push   %edi
80106f14:	56                   	push   %esi
80106f15:	53                   	push   %ebx
80106f16:	83 ec 1c             	sub    $0x1c,%esp
  pde_t *d;
  pte_t *pte;
  uint pa, i, flags;
  char *mem;

  if((d = setupkvm()) == 0)
80106f19:	e8 02 ff ff ff       	call   80106e20 <setupkvm>
80106f1e:	89 45 e0             	mov    %eax,-0x20(%ebp)
80106f21:	85 c0                	test   %eax,%eax
80106f23:	0f 84 e9 00 00 00    	je     80107012 <copyuvm+0x102>
    return 0;
  for(i = 0; i < sz; i += PGSIZE){
80106f29:	8b 4d 0c             	mov    0xc(%ebp),%ecx
80106f2c:	85 c9                	test   %ecx,%ecx
80106f2e:	0f 84 b2 00 00 00    	je     80106fe6 <copyuvm+0xd6>
80106f34:	31 f6                	xor    %esi,%esi
80106f36:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80106f3d:	00 
80106f3e:	66 90                	xchg   %ax,%ax
  if(*pde & PTE_P){
80106f40:	8b 4d 08             	mov    0x8(%ebp),%ecx
  pde = &pgdir[PDX(va)];
80106f43:	89 f0                	mov    %esi,%eax
80106f45:	c1 e8 16             	shr    $0x16,%eax
  if(*pde & PTE_P){
80106f48:	8b 04 81             	mov    (%ecx,%eax,4),%eax
80106f4b:	a8 01                	test   $0x1,%al
80106f4d:	75 11                	jne    80106f60 <copyuvm+0x50>
    if((pte = walkpgdir(pgdir, (void *) i, 0)) == 0)
      panic("copyuvm: pte should exist");
80106f4f:	83 ec 0c             	sub    $0xc,%esp
80106f52:	68 2c 76 10 80       	push   $0x8010762c
80106f57:	e8 24 94 ff ff       	call   80100380 <panic>
80106f5c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
  return &pgtab[PTX(va)];
80106f60:	89 f2                	mov    %esi,%edx
    pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
80106f62:	25 00 f0 ff ff       	and    $0xfffff000,%eax
  return &pgtab[PTX(va)];
80106f67:	c1 ea 0a             	shr    $0xa,%edx
80106f6a:	81 e2 fc 0f 00 00    	and    $0xffc,%edx
80106f70:	8d 84 10 00 00 00 80 	lea    -0x80000000(%eax,%edx,1),%eax
    if((pte = walkpgdir(pgdir, (void *) i, 0)) == 0)
80106f77:	85 c0                	test   %eax,%eax
80106f79:	74 d4                	je     80106f4f <copyuvm+0x3f>
    if(!(*pte & PTE_P))
80106f7b:	8b 00                	mov    (%eax),%eax
80106f7d:	a8 01                	test   $0x1,%al
80106f7f:	0f 84 9f 00 00 00    	je     80107024 <copyuvm+0x114>
      panic("copyuvm: page not present");
    pa = PTE_ADDR(*pte);
80106f85:	89 c7                	mov    %eax,%edi
    flags = PTE_FLAGS(*pte);
80106f87:	25 ff 0f 00 00       	and    $0xfff,%eax
80106f8c:	89 45 e4             	mov    %eax,-0x1c(%ebp)
    pa = PTE_ADDR(*pte);
80106f8f:	81 e7 00 f0 ff ff    	and    $0xfffff000,%edi
    if((mem = kalloc()) == 0)
80106f95:	e8 c6 b6 ff ff       	call   80102660 <kalloc>
80106f9a:	89 c3                	mov    %eax,%ebx
80106f9c:	85 c0                	test   %eax,%eax
80106f9e:	74 64                	je     80107004 <copyuvm+0xf4>
      goto bad;
    memmove(mem, (char*)P2V(pa), PGSIZE);
80106fa0:	83 ec 04             	sub    $0x4,%esp
80106fa3:	81 c7 00 00 00 80    	add    $0x80000000,%edi
80106fa9:	68 00 10 00 00       	push   $0x1000
80106fae:	57                   	push   %edi
80106faf:	50                   	push   %eax
80106fb0:	e8 7b d7 ff ff       	call   80104730 <memmove>
    if(mappages(d, (void*)i, PGSIZE, V2P(mem), flags) < 0) {
80106fb5:	58                   	pop    %eax
80106fb6:	8d 83 00 00 00 80    	lea    -0x80000000(%ebx),%eax
80106fbc:	5a                   	pop    %edx
80106fbd:	ff 75 e4             	push   -0x1c(%ebp)
80106fc0:	b9 00 10 00 00       	mov    $0x1000,%ecx
80106fc5:	89 f2                	mov    %esi,%edx
80106fc7:	50                   	push   %eax
80106fc8:	8b 45 e0             	mov    -0x20(%ebp),%eax
80106fcb:	e8 90 f8 ff ff       	call   80106860 <mappages>
80106fd0:	83 c4 10             	add    $0x10,%esp
80106fd3:	85 c0                	test   %eax,%eax
80106fd5:	78 21                	js     80106ff8 <copyuvm+0xe8>
  for(i = 0; i < sz; i += PGSIZE){
80106fd7:	81 c6 00 10 00 00    	add    $0x1000,%esi
80106fdd:	3b 75 0c             	cmp    0xc(%ebp),%esi
80106fe0:	0f 82 5a ff ff ff    	jb     80106f40 <copyuvm+0x30>
  return d;

bad:
  freevm(d);
  return 0;
}
80106fe6:	8b 45 e0             	mov    -0x20(%ebp),%eax
80106fe9:	8d 65 f4             	lea    -0xc(%ebp),%esp
80106fec:	5b                   	pop    %ebx
80106fed:	5e                   	pop    %esi
80106fee:	5f                   	pop    %edi
80106fef:	5d                   	pop    %ebp
80106ff0:	c3                   	ret
80106ff1:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
      kfree(mem);
80106ff8:	83 ec 0c             	sub    $0xc,%esp
80106ffb:	53                   	push   %ebx
80106ffc:	e8 9f b4 ff ff       	call   801024a0 <kfree>
      goto bad;
80107001:	83 c4 10             	add    $0x10,%esp
  freevm(d);
80107004:	83 ec 0c             	sub    $0xc,%esp
80107007:	ff 75 e0             	push   -0x20(%ebp)
8010700a:	e8 91 fd ff ff       	call   80106da0 <freevm>
  return 0;
8010700f:	83 c4 10             	add    $0x10,%esp
    return 0;
80107012:	c7 45 e0 00 00 00 00 	movl   $0x0,-0x20(%ebp)
}
80107019:	8b 45 e0             	mov    -0x20(%ebp),%eax
8010701c:	8d 65 f4             	lea    -0xc(%ebp),%esp
8010701f:	5b                   	pop    %ebx
80107020:	5e                   	pop    %esi
80107021:	5f                   	pop    %edi
80107022:	5d                   	pop    %ebp
80107023:	c3                   	ret
      panic("copyuvm: page not present");
80107024:	83 ec 0c             	sub    $0xc,%esp
80107027:	68 46 76 10 80       	push   $0x80107646
8010702c:	e8 4f 93 ff ff       	call   80100380 <panic>
80107031:	2e 8d b4 26 00 00 00 	lea    %cs:0x0(%esi,%eiz,1),%esi
80107038:	00 
80107039:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi

80107040 <uva2ka>:

//PAGEBREAK!
// Map user virtual address to kernel address.
char*
uva2ka(pde_t *pgdir, char *uva)
{
80107040:	55                   	push   %ebp
80107041:	89 e5                	mov    %esp,%ebp
80107043:	8b 45 0c             	mov    0xc(%ebp),%eax
  if(*pde & PTE_P){
80107046:	8b 55 08             	mov    0x8(%ebp),%edx
  pde = &pgdir[PDX(va)];
80107049:	89 c1                	mov    %eax,%ecx
8010704b:	c1 e9 16             	shr    $0x16,%ecx
  if(*pde & PTE_P){
8010704e:	8b 14 8a             	mov    (%edx,%ecx,4),%edx
80107051:	f6 c2 01             	test   $0x1,%dl
80107054:	0f 84 f8 00 00 00    	je     80107152 <uva2ka.cold>
  return &pgtab[PTX(va)];
8010705a:	c1 e8 0c             	shr    $0xc,%eax
    pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
8010705d:	81 e2 00 f0 ff ff    	and    $0xfffff000,%edx
  if((*pte & PTE_P) == 0)
    return 0;
  if((*pte & PTE_U) == 0)
    return 0;
  return (char*)P2V(PTE_ADDR(*pte));
}
80107063:	5d                   	pop    %ebp
  return &pgtab[PTX(va)];
80107064:	25 ff 03 00 00       	and    $0x3ff,%eax
  if((*pte & PTE_P) == 0)
80107069:	8b 94 82 00 00 00 80 	mov    -0x80000000(%edx,%eax,4),%edx
  return (char*)P2V(PTE_ADDR(*pte));
80107070:	89 d0                	mov    %edx,%eax
80107072:	f7 d2                	not    %edx
80107074:	25 00 f0 ff ff       	and    $0xfffff000,%eax
80107079:	05 00 00 00 80       	add    $0x80000000,%eax
8010707e:	83 e2 05             	and    $0x5,%edx
80107081:	ba 00 00 00 00       	mov    $0x0,%edx
80107086:	0f 45 c2             	cmovne %edx,%eax
}
80107089:	c3                   	ret
8010708a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi

80107090 <copyout>:
// Copy len bytes from p to user address va in page table pgdir.
// Most useful when pgdir is not the current page table.
// uva2ka ensures this only works for PTE_U pages.
int
copyout(pde_t *pgdir, uint va, void *p, uint len)
{
80107090:	55                   	push   %ebp
80107091:	89 e5                	mov    %esp,%ebp
80107093:	57                   	push   %edi
80107094:	56                   	push   %esi
80107095:	53                   	push   %ebx
80107096:	83 ec 0c             	sub    $0xc,%esp
80107099:	8b 75 14             	mov    0x14(%ebp),%esi
8010709c:	8b 45 0c             	mov    0xc(%ebp),%eax
8010709f:	8b 55 10             	mov    0x10(%ebp),%edx
  char *buf, *pa0;
  uint n, va0;

  buf = (char*)p;
  while(len > 0){
801070a2:	85 f6                	test   %esi,%esi
801070a4:	75 51                	jne    801070f7 <copyout+0x67>
801070a6:	e9 9d 00 00 00       	jmp    80107148 <copyout+0xb8>
801070ab:	2e 8d 74 26 00       	lea    %cs:0x0(%esi,%eiz,1),%esi
  return (char*)P2V(PTE_ADDR(*pte));
801070b0:	81 e3 00 f0 ff ff    	and    $0xfffff000,%ebx
801070b6:	8d 8b 00 00 00 80    	lea    -0x80000000(%ebx),%ecx
    va0 = (uint)PGROUNDDOWN(va);
    pa0 = uva2ka(pgdir, (char*)va0);
    if(pa0 == 0)
801070bc:	81 fb 00 00 00 80    	cmp    $0x80000000,%ebx
801070c2:	74 74                	je     80107138 <copyout+0xa8>
      return -1;
    n = PGSIZE - (va - va0);
801070c4:	89 fb                	mov    %edi,%ebx
801070c6:	29 c3                	sub    %eax,%ebx
801070c8:	81 c3 00 10 00 00    	add    $0x1000,%ebx
    if(n > len)
801070ce:	39 f3                	cmp    %esi,%ebx
801070d0:	0f 47 de             	cmova  %esi,%ebx
      n = len;
    memmove(pa0 + (va - va0), buf, n);
801070d3:	29 f8                	sub    %edi,%eax
801070d5:	83 ec 04             	sub    $0x4,%esp
801070d8:	01 c1                	add    %eax,%ecx
801070da:	53                   	push   %ebx
801070db:	52                   	push   %edx
801070dc:	89 55 10             	mov    %edx,0x10(%ebp)
801070df:	51                   	push   %ecx
801070e0:	e8 4b d6 ff ff       	call   80104730 <memmove>
    len -= n;
    buf += n;
801070e5:	8b 55 10             	mov    0x10(%ebp),%edx
    va = va0 + PGSIZE;
801070e8:	8d 87 00 10 00 00    	lea    0x1000(%edi),%eax
  while(len > 0){
801070ee:	83 c4 10             	add    $0x10,%esp
    buf += n;
801070f1:	01 da                	add    %ebx,%edx
  while(len > 0){
801070f3:	29 de                	sub    %ebx,%esi
801070f5:	74 51                	je     80107148 <copyout+0xb8>
  if(*pde & PTE_P){
801070f7:	8b 5d 08             	mov    0x8(%ebp),%ebx
  pde = &pgdir[PDX(va)];
801070fa:	89 c1                	mov    %eax,%ecx
    va0 = (uint)PGROUNDDOWN(va);
801070fc:	89 c7                	mov    %eax,%edi
  pde = &pgdir[PDX(va)];
801070fe:	c1 e9 16             	shr    $0x16,%ecx
    va0 = (uint)PGROUNDDOWN(va);
80107101:	81 e7 00 f0 ff ff    	and    $0xfffff000,%edi
  if(*pde & PTE_P){
80107107:	8b 0c 8b             	mov    (%ebx,%ecx,4),%ecx
8010710a:	f6 c1 01             	test   $0x1,%cl
8010710d:	0f 84 46 00 00 00    	je     80107159 <copyout.cold>
  return &pgtab[PTX(va)];
80107113:	89 fb                	mov    %edi,%ebx
    pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
80107115:	81 e1 00 f0 ff ff    	and    $0xfffff000,%ecx
  return &pgtab[PTX(va)];
8010711b:	c1 eb 0c             	shr    $0xc,%ebx
8010711e:	81 e3 ff 03 00 00    	and    $0x3ff,%ebx
  if((*pte & PTE_P) == 0)
80107124:	8b 9c 99 00 00 00 80 	mov    -0x80000000(%ecx,%ebx,4),%ebx
  if((*pte & PTE_U) == 0)
8010712b:	89 d9                	mov    %ebx,%ecx
8010712d:	f7 d1                	not    %ecx
8010712f:	83 e1 05             	and    $0x5,%ecx
80107132:	0f 84 78 ff ff ff    	je     801070b0 <copyout+0x20>
  }
  return 0;
}
80107138:	8d 65 f4             	lea    -0xc(%ebp),%esp
      return -1;
8010713b:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
}
80107140:	5b                   	pop    %ebx
80107141:	5e                   	pop    %esi
80107142:	5f                   	pop    %edi
80107143:	5d                   	pop    %ebp
80107144:	c3                   	ret
80107145:	8d 76 00             	lea    0x0(%esi),%esi
80107148:	8d 65 f4             	lea    -0xc(%ebp),%esp
  return 0;
8010714b:	31 c0                	xor    %eax,%eax
}
8010714d:	5b                   	pop    %ebx
8010714e:	5e                   	pop    %esi
8010714f:	5f                   	pop    %edi
80107150:	5d                   	pop    %ebp
80107151:	c3                   	ret

80107152 <uva2ka.cold>:
  if((*pte & PTE_P) == 0)
80107152:	a1 00 00 00 00       	mov    0x0,%eax
80107157:	0f 0b                	ud2

80107159 <copyout.cold>:
80107159:	a1 00 00 00 00       	mov    0x0,%eax
8010715e:	0f 0b                	ud2
