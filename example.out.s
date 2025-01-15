	.text
	.globl main
	.attribute arch, "rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0_zicsr2p0_zifencei2p0_zba1p0_zbb1p0"
long_func:
.long_func_0:
	sd			s4,-48(sp)
	sd			s3,-40(sp)
	sd			s2,-32(sp)
	sd			s1,-24(sp)
	sd			fp,-16(sp)
	sd			ra,-8(sp)
	addi		sp,sp,-48
.long_func_1:
	addiw		t0,x0,0
	addiw		t1,x0,0
	addiw		t2,x0,0
	addiw		t3,x0,0
	addiw		t4,x0,0
	addiw		t4,x0,2
	addiw		t5,x0,0
	addiw		t6,x0,1
	add			fp,t6,x0
	add			s1,t3,x0
	add			s2,t2,x0
	add			s3,t1,x0
.long_func_2:
	addiw		t0,x0,0
	bgt			t5,t0,.long_func_3
.long_func_4:
	add			a0,fp,x0
	call		putint
	addiw		t0,x0,10
	add			a0,t0,x0
	call		putch
	addiw		t0,x0,2
	addiw		t1,x0,1
	addiw		t2,x0,1
	add			s4,t2,x0
	add			t3,t0,x0
	add			t0,fp,x0
.long_func_183:
	addiw		t0,x0,0
	bgt			t1,t0,.long_func_184
.long_func_185:
	add			a0,s4,x0
	call		putint
	addiw		t0,x0,10
	add			a0,t0,x0
	call		putch
	addiw		t0,x0,2
	add			fp,t0,x0
	add			t0,s1,x0
	add			t1,s2,x0
	add			t2,s3,x0
	add			t3,s4,x0
.long_func_364:
	addiw		t4,x0,16
	blt			fp,t4,.long_func_365
.long_func_366:
	addiw		t4,x0,0
	add			t5,t1,x0
	add			t6,t2,x0
	add			a0,t3,x0
.long_func_548:
	addiw		t1,x0,16
	blt			t4,t1,.long_func_549
.long_func_550:
	addiw		t0,x0,0
	add			a0,t0,x0
	addi		sp,sp,48
	ld			ra,-8(sp)
	ld			fp,-16(sp)
	ld			s1,-24(sp)
	ld			s2,-32(sp)
	ld			s3,-40(sp)
	ld			s4,-48(sp)
	jalr		x0,ra,0
.long_func_549:
	addiw		a2,x0,2
	addiw		a1,x0,1
	add			t1,t0,x0
	add			t2,t5,x0
	add			t3,t6,x0
	add			t5,t4,x0
	add			t0,a0,x0
.long_func_551:
	addiw		t0,x0,0
	bgt			t5,t0,.long_func_552
.long_func_553:
	add			t0,t4,x0
	lui			t5,%hi(SHIFT_TABLE)
	addi		t5,t5,%lo(SHIFT_TABLE)
	slli		t0,t0,2
	add			t0,t5,t0
	lw			t0,0(t0)
	bne			t0,a1,.long_func_732
.long_func_733:
	addiw		t0,x0,1
	addw		t0,t4,t0
	add			t4,t0,x0
	add			t0,t1,x0
	add			t5,t2,x0
	add			t6,t3,x0
	add			a0,a1,x0
	jal			x0,.long_func_548
.long_func_732:
	addiw		t0,x0,1
	add			a0,t0,x0
	addi		sp,sp,48
	ld			ra,-8(sp)
	ld			fp,-16(sp)
	ld			s1,-24(sp)
	ld			s2,-32(sp)
	ld			s3,-40(sp)
	ld			s4,-48(sp)
	jalr		x0,ra,0
.long_func_552:
	addiw		t0,x0,0
	addiw		t1,x0,0
	addiw		t2,x0,1
	add			t3,t5,x0
	add			t6,t1,x0
	add			t1,t0,x0
.long_func_554:
	addiw		t0,x0,16
	blt			t6,t0,.long_func_555
.long_func_556:
	bne			t1,x0,.long_func_560
.long_func_835:
	add			t0,a1,x0
	add			a1,t3,x0
	add			t3,t6,x0
.long_func_561:
	addiw		t6,x0,0
	add			a0,t6,x0
	add			t6,a2,x0
	add			a3,t2,x0
.long_func_641:
	bne			t6,x0,.long_func_642
.long_func_643:
	addiw		t1,x0,1
	addiw		t2,x0,15
	bge			t1,t2,.long_func_720
.long_func_721:
	addiw		t2,x0,0
	bgt			t1,t2,.long_func_726
.long_func_727:
	add			t2,t5,x0
.long_func_728:
	add			t6,t5,x0
.long_func_722:
	add			a1,t0,x0
	add			t5,t6,x0
	add			a2,a0,x0
	add			t0,t6,x0
	jal			x0,.long_func_551
.long_func_726:
	lui			t2,8
	addiw		t2,t2,-1
	bgt			t5,t2,.long_func_729
.long_func_730:
	add			t2,t1,x0
	lui			t6,%hi(SHIFT_TABLE)
	addi		t6,t6,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,t6,t2
	lw			t2,0(t2)
	divw		t6,t5,t2
	add			t2,t5,x0
	add			t5,t6,x0
.long_func_731:
	jal			x0,.long_func_728
.long_func_729:
	add			t2,t1,x0
	lui			t6,%hi(SHIFT_TABLE)
	addi		t6,t6,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,t6,t2
	lw			t2,0(t2)
	divw		t2,t5,t2
	lui			t5,16
	addw		t5,t2,t5
	addiw		t6,x0,15
	subw		t6,t6,t1
	addiw		a1,x0,1
	addw		t6,t6,a1
	lui			a1,%hi(SHIFT_TABLE)
	addi		a1,a1,%lo(SHIFT_TABLE)
	slli		t6,t6,2
	add			t6,a1,t6
	lw			t6,0(t6)
	subw		t5,t5,t6
	jal			x0,.long_func_731
.long_func_720:
	addiw		t2,x0,0
	blt			t5,t2,.long_func_723
.long_func_724:
	addiw		t2,x0,0
.long_func_725:
	add			t6,t2,x0
	add			t2,t5,x0
	jal			x0,.long_func_722
.long_func_723:
	lui			t2,16
	addiw		t2,t2,-1
	jal			x0,.long_func_725
.long_func_642:
	addiw		t1,x0,0
	addiw		t2,x0,0
	addiw		t3,x0,1
	add			a1,t6,x0
	add			a3,t2,x0
	add			t2,t1,x0
.long_func_644:
	addiw		t1,x0,16
	blt			a3,t1,.long_func_645
.long_func_646:
	bne			t2,x0,.long_func_650
.long_func_853:
	add			t1,a0,x0
	add			a4,t3,x0
	add			a5,a1,x0
	add			t3,a3,x0
.long_func_651:
	addiw		a0,x0,0
	add			a1,a2,x0
	add			a0,a2,x0
	add			a3,a4,x0
	add			a3,a5,x0
.long_func_680:
	bne			a0,x0,.long_func_681
.long_func_682:
	addiw		a3,x0,1
	addiw		t2,x0,15
	bge			a3,t2,.long_func_708
.long_func_709:
	addiw		t2,x0,0
	bgt			a3,t2,.long_func_714
.long_func_715:
	add			a0,t6,x0
	add			t2,t6,x0
.long_func_716:
	add			a4,a0,x0
.long_func_710:
	add			a0,t1,x0
	add			t6,t2,x0
	add			a2,a1,x0
	add			a1,a4,x0
	add			t1,t2,x0
	jal			x0,.long_func_641
.long_func_714:
	lui			t2,8
	addiw		t2,t2,-1
	bgt			t6,t2,.long_func_717
.long_func_718:
	add			t2,a3,x0
	lui			a0,%hi(SHIFT_TABLE)
	addi		a0,a0,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,a0,t2
	lw			t2,0(t2)
	divw		a0,t6,t2
	add			t2,t6,x0
	add			t6,a0,x0
.long_func_719:
	add			a0,t2,x0
	add			t2,t6,x0
	jal			x0,.long_func_716
.long_func_717:
	add			t2,a3,x0
	lui			a0,%hi(SHIFT_TABLE)
	addi		a0,a0,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,a0,t2
	lw			t2,0(t2)
	divw		t2,t6,t2
	lui			t6,16
	addw		t6,t2,t6
	addiw		a0,x0,15
	subw		a0,a0,a3
	addiw		a2,x0,1
	addw		a0,a0,a2
	lui			a2,%hi(SHIFT_TABLE)
	addi		a2,a2,%lo(SHIFT_TABLE)
	slli		a0,a0,2
	add			a0,a2,a0
	lw			a0,0(a0)
	subw		t6,t6,a0
	jal			x0,.long_func_719
.long_func_708:
	addiw		t2,x0,0
	blt			t6,t2,.long_func_711
.long_func_712:
	addiw		t2,x0,0
.long_func_713:
	add			a4,t6,x0
	jal			x0,.long_func_710
.long_func_711:
	lui			t2,16
	addiw		t2,t2,-1
	jal			x0,.long_func_713
.long_func_681:
	addiw		t2,x0,0
	addiw		t3,x0,0
	add			a2,a0,x0
	add			a3,a1,x0
	add			a4,t3,x0
.long_func_683:
	addiw		t3,x0,16
	blt			a4,t3,.long_func_684
.long_func_685:
	addiw		t3,x0,0
	addiw		a2,x0,0
	add			a5,a2,x0
.long_func_693:
	addiw		a2,x0,16
	blt			a5,a2,.long_func_694
.long_func_695:
	addiw		a2,x0,1
	addiw		a3,x0,15
	bgt			a2,a3,.long_func_699
.long_func_700:
	addiw		a0,x0,0
	addiw		a1,x0,0
	addiw		a2,x0,1
	lui			a3,%hi(SHIFT_TABLE)
	addi		a3,a3,%lo(SHIFT_TABLE)
	slli		a2,a2,2
	add			a2,a3,a2
	lw			a2,0(a2)
	mulw		t3,t3,a2
	lui			a2,16
	addiw		a2,a2,-1
	add			a3,a2,x0
.long_func_702:
	addiw		a2,x0,16
	blt			a1,a2,.long_func_703
.long_func_704:
	add			a2,a0,x0
	add			a4,t3,x0
	add			t3,a1,x0
.long_func_701:
	add			a1,t2,x0
	add			a0,t2,x0
	add			a0,a2,x0
	add			a3,a4,x0
	add			t2,a2,x0
	jal			x0,.long_func_680
.long_func_703:
	addiw		a2,x0,2
	srliw		a2,t3,31
	add			a2,a2,t3
	andi		a2,a2,-2
	subw		a2,t3,a2
	bne			a2,x0,.long_func_707
.long_func_864:
	add			a2,a0,x0
.long_func_706:
	addiw		a0,x0,2
	srliw		a0,t3,31
	add			t3,t3,a0
	sraiw		a0,t3,1
	addiw		t3,x0,2
	srliw		t3,a3,31
	add			t3,a3,t3
	sraiw		t3,t3,1
	addiw		a3,x0,1
	addw		a1,a1,a3
	add			a3,t3,x0
	add			t3,a0,x0
	add			a0,a2,x0
	jal			x0,.long_func_702
.long_func_707:
	addiw		a2,x0,2
	srliw		a2,a3,31
	add			a2,a2,a3
	andi		a2,a2,-2
	subw		a2,a3,a2
	bne			a2,x0,.long_func_705
.long_func_865:
	add			a2,a0,x0
	jal			x0,.long_func_706
.long_func_705:
	addiw		a2,x0,1
	add			a2,a1,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		a2,a2,2
	add			a2,a4,a2
	lw			a2,0(a2)
	addw		a0,a0,a2
	add			a2,a0,x0
	jal			x0,.long_func_706
.long_func_699:
	addiw		t3,x0,0
	add			a2,t3,x0
	add			a3,a0,x0
	add			a4,a1,x0
	add			t3,a5,x0
	jal			x0,.long_func_701
.long_func_694:
	addiw		a2,x0,2
	srliw		a2,a1,31
	add			a2,a2,a1
	andi		a2,a2,-2
	subw		a2,a1,a2
	bne			a2,x0,.long_func_698
.long_func_862:
	add			a3,t3,x0
.long_func_697:
	addiw		t3,x0,2
	srliw		t3,a1,31
	add			t3,a1,t3
	sraiw		a1,t3,1
	addiw		t3,x0,2
	srliw		t3,a0,31
	add			t3,a0,t3
	sraiw		t3,t3,1
	addiw		a0,x0,1
	addw		a2,a5,a0
	add			a0,t3,x0
	add			a5,a2,x0
	add			t3,a3,x0
	jal			x0,.long_func_693
.long_func_698:
	addiw		a2,x0,2
	srliw		a2,a0,31
	add			a2,a2,a0
	andi		a2,a2,-2
	subw		a2,a0,a2
	bne			a2,x0,.long_func_696
.long_func_863:
	add			a3,t3,x0
	jal			x0,.long_func_697
.long_func_696:
	addiw		a2,x0,1
	add			a2,a5,x0
	lui			a3,%hi(SHIFT_TABLE)
	addi		a3,a3,%lo(SHIFT_TABLE)
	slli		a2,a2,2
	add			a2,a3,a2
	lw			a2,0(a2)
	addw		t3,t3,a2
	add			a3,t3,x0
	jal			x0,.long_func_697
.long_func_684:
	addiw		t3,x0,2
	srliw		t3,a3,31
	add			t3,t3,a3
	andi		t3,t3,-2
	subw		t3,a3,t3
	bne			t3,x0,.long_func_686
.long_func_687:
	addiw		t3,x0,2
	srliw		t3,a2,31
	add			t3,t3,a2
	andi		t3,t3,-2
	subw		t3,a2,t3
	bne			t3,x0,.long_func_691
.long_func_861:
.long_func_692:
.long_func_688:
	addiw		t3,x0,2
	srliw		t3,a3,31
	add			t3,a3,t3
	sraiw		a3,t3,1
	addiw		t3,x0,2
	srliw		t3,a2,31
	add			t3,a2,t3
	sraiw		t3,t3,1
	addiw		a2,x0,1
	addw		a4,a4,a2
	add			a2,t3,x0
	jal			x0,.long_func_683
.long_func_691:
	addiw		t3,x0,1
	add			t3,a4,x0
	lui			a5,%hi(SHIFT_TABLE)
	addi		a5,a5,%lo(SHIFT_TABLE)
	slli		t3,t3,2
	add			t3,a5,t3
	lw			t3,0(t3)
	addw		t2,t2,t3
	jal			x0,.long_func_692
.long_func_686:
	addiw		t3,x0,2
	srliw		t3,a2,31
	add			t3,t3,a2
	andi		t3,t3,-2
	subw		t3,a2,t3
	addiw		a5,x0,0
	beq			t3,a5,.long_func_689
.long_func_860:
.long_func_690:
	jal			x0,.long_func_688
.long_func_689:
	addiw		t3,x0,1
	add			t3,a4,x0
	lui			a5,%hi(SHIFT_TABLE)
	addi		a5,a5,%lo(SHIFT_TABLE)
	slli		t3,t3,2
	add			t3,a5,t3
	lw			t3,0(t3)
	addw		t2,t2,t3
	jal			x0,.long_func_690
.long_func_650:
	addiw		t1,x0,0
	add			a4,a2,x0
	add			t1,t2,x0
.long_func_652:
	bne			a4,x0,.long_func_653
.long_func_654:
	add			t1,a0,x0
	add			a4,t3,x0
	add			a5,a1,x0
	add			t3,a3,x0
	add			t2,a0,x0
	jal			x0,.long_func_651
.long_func_653:
	addiw		t1,x0,0
	addiw		t2,x0,0
	add			t3,a4,x0
	add			a1,a0,x0
	add			a3,t2,x0
.long_func_655:
	addiw		t2,x0,16
	blt			a3,t2,.long_func_656
.long_func_657:
	addiw		t2,x0,0
	addiw		t3,x0,0
	add			a1,a4,x0
	add			a3,t3,x0
.long_func_665:
	addiw		t3,x0,16
	blt			a3,t3,.long_func_666
.long_func_667:
	addiw		t3,x0,1
	addiw		a4,x0,15
	bgt			t3,a4,.long_func_671
.long_func_672:
	addiw		t3,x0,0
	addiw		a0,x0,0
	addiw		a1,x0,1
	lui			a3,%hi(SHIFT_TABLE)
	addi		a3,a3,%lo(SHIFT_TABLE)
	slli		a1,a1,2
	add			a1,a3,a1
	lw			a1,0(a1)
	mulw		t2,t2,a1
	lui			a1,16
	addiw		a1,a1,-1
	add			a3,t2,x0
	add			t2,t3,x0
.long_func_674:
	addiw		t3,x0,16
	blt			a0,t3,.long_func_675
.long_func_676:
	add			t3,a1,x0
	add			a1,a3,x0
	add			a3,a0,x0
.long_func_673:
	add			a0,t1,x0
	add			a4,t2,x0
	add			t1,t2,x0
	jal			x0,.long_func_652
.long_func_675:
	addiw		t3,x0,2
	srliw		t3,a3,31
	add			t3,t3,a3
	andi		t3,t3,-2
	subw		t3,a3,t3
	bne			t3,x0,.long_func_679
.long_func_858:
	add			a4,t2,x0
.long_func_678:
	addiw		t2,x0,2
	srliw		t2,a3,31
	add			t2,a3,t2
	sraiw		t3,t2,1
	addiw		t2,x0,2
	srliw		t2,a1,31
	add			t2,a1,t2
	sraiw		t2,t2,1
	addiw		a1,x0,1
	addw		a0,a0,a1
	add			a1,t2,x0
	add			a3,t3,x0
	add			t2,a4,x0
	jal			x0,.long_func_674
.long_func_679:
	addiw		t3,x0,2
	srliw		t3,a1,31
	add			t3,t3,a1
	andi		t3,t3,-2
	subw		t3,a1,t3
	bne			t3,x0,.long_func_677
.long_func_859:
	add			a4,t2,x0
	jal			x0,.long_func_678
.long_func_677:
	addiw		t3,x0,1
	add			t3,a0,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t3,t3,2
	add			t3,a4,t3
	lw			t3,0(t3)
	addw		t2,t2,t3
	add			a4,t2,x0
	jal			x0,.long_func_678
.long_func_671:
	addiw		t2,x0,0
	add			t3,a1,x0
	add			a1,a0,x0
	jal			x0,.long_func_673
.long_func_666:
	addiw		t3,x0,2
	srliw		t3,a0,31
	add			t3,t3,a0
	andi		t3,t3,-2
	subw		t3,a0,t3
	bne			t3,x0,.long_func_670
.long_func_856:
	add			a4,t2,x0
.long_func_669:
	addiw		t2,x0,2
	srliw		t2,a0,31
	add			t2,a0,t2
	sraiw		t3,t2,1
	addiw		t2,x0,2
	srliw		t2,a1,31
	add			t2,a1,t2
	sraiw		t2,t2,1
	addiw		a0,x0,1
	addw		a3,a3,a0
	add			a1,t2,x0
	add			a0,t3,x0
	add			t2,a4,x0
	jal			x0,.long_func_665
.long_func_670:
	addiw		t3,x0,2
	srliw		t3,a1,31
	add			t3,t3,a1
	andi		t3,t3,-2
	subw		t3,a1,t3
	bne			t3,x0,.long_func_668
.long_func_857:
	add			a4,t2,x0
	jal			x0,.long_func_669
.long_func_668:
	addiw		t3,x0,1
	add			t3,a3,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t3,t3,2
	add			t3,a4,t3
	lw			t3,0(t3)
	addw		t2,t2,t3
	add			a4,t2,x0
	jal			x0,.long_func_669
.long_func_656:
	addiw		t2,x0,2
	srliw		t2,a1,31
	add			t2,t2,a1
	andi		t2,t2,-2
	subw		t2,a1,t2
	bne			t2,x0,.long_func_658
.long_func_659:
	addiw		t2,x0,2
	srliw		t2,t3,31
	add			t2,t2,t3
	andi		t2,t2,-2
	subw		t2,t3,t2
	bne			t2,x0,.long_func_663
.long_func_855:
.long_func_664:
.long_func_660:
	addiw		t2,x0,2
	srliw		t2,a1,31
	add			t2,a1,t2
	sraiw		a1,t2,1
	addiw		t2,x0,2
	srliw		t2,t3,31
	add			t2,t3,t2
	sraiw		t2,t2,1
	addiw		t3,x0,1
	addw		a3,a3,t3
	add			t3,t2,x0
	jal			x0,.long_func_655
.long_func_663:
	addiw		t2,x0,1
	add			t2,a3,x0
	lui			a5,%hi(SHIFT_TABLE)
	addi		a5,a5,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,a5,t2
	lw			t2,0(t2)
	addw		t1,t1,t2
	jal			x0,.long_func_664
.long_func_658:
	addiw		t2,x0,2
	srliw		t2,t3,31
	add			t2,t2,t3
	andi		t2,t2,-2
	subw		t2,t3,t2
	addiw		a5,x0,0
	beq			t2,a5,.long_func_661
.long_func_854:
.long_func_662:
	jal			x0,.long_func_660
.long_func_661:
	addiw		t2,x0,1
	add			t2,a3,x0
	lui			a5,%hi(SHIFT_TABLE)
	addi		a5,a5,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,a5,t2
	lw			t2,0(t2)
	addw		t1,t1,t2
	jal			x0,.long_func_662
.long_func_645:
	addiw		t1,x0,2
	srliw		t1,a1,31
	add			t1,t1,a1
	andi		t1,t1,-2
	subw		t1,a1,t1
	bne			t1,x0,.long_func_649
.long_func_851:
	add			a4,t2,x0
.long_func_648:
	addiw		t1,x0,2
	srliw		t1,a1,31
	add			t1,a1,t1
	sraiw		t2,t1,1
	addiw		t1,x0,2
	srliw		t1,t3,31
	add			t1,t3,t1
	sraiw		t1,t1,1
	addiw		t3,x0,1
	addw		a3,a3,t3
	add			t3,t1,x0
	add			a1,t2,x0
	add			t2,a4,x0
	jal			x0,.long_func_644
.long_func_649:
	addiw		t1,x0,2
	srliw		t1,t3,31
	add			t1,t1,t3
	andi		t1,t1,-2
	subw		t1,t3,t1
	bne			t1,x0,.long_func_647
.long_func_852:
	add			a4,t2,x0
	jal			x0,.long_func_648
.long_func_647:
	addiw		t1,x0,1
	add			t1,a3,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t1,t1,2
	add			t1,a4,t1
	lw			t1,0(t1)
	addw		t1,t2,t1
	add			a4,t1,x0
	jal			x0,.long_func_648
.long_func_560:
	addiw		t0,x0,0
	add			a0,t0,x0
	add			a3,a2,x0
	add			t0,t1,x0
.long_func_562:
	bne			a3,x0,.long_func_563
.long_func_564:
	add			t0,a0,x0
	add			a1,t3,x0
	add			t3,t6,x0
	add			t1,a0,x0
	jal			x0,.long_func_561
.long_func_563:
	addiw		t0,x0,0
	addiw		t1,x0,0
	addiw		t2,x0,1
	add			t3,a3,x0
	add			t6,t1,x0
	add			t1,t0,x0
.long_func_565:
	addiw		t0,x0,16
	blt			t6,t0,.long_func_566
.long_func_567:
	bne			t1,x0,.long_func_571
.long_func_838:
	add			t0,a0,x0
.long_func_572:
	addiw		a0,x0,0
	add			a4,a1,x0
	add			a0,a1,x0
	add			a1,t2,x0
.long_func_601:
	bne			a0,x0,.long_func_602
.long_func_603:
	addiw		t2,x0,1
	addiw		t1,x0,15
	bge			t2,t1,.long_func_629
.long_func_630:
	addiw		t1,x0,0
	bgt			t2,t1,.long_func_635
.long_func_636:
	add			t3,a3,x0
	add			t1,a3,x0
.long_func_637:
.long_func_631:
	add			a0,t0,x0
	add			a3,t1,x0
	add			a1,a4,x0
	add			t0,t1,x0
	jal			x0,.long_func_562
.long_func_635:
	lui			t1,8
	addiw		t1,t1,-1
	bgt			a3,t1,.long_func_638
.long_func_639:
	add			t1,t2,x0
	lui			t3,%hi(SHIFT_TABLE)
	addi		t3,t3,%lo(SHIFT_TABLE)
	slli		t1,t1,2
	add			t1,t3,t1
	lw			t1,0(t1)
	divw		t3,a3,t1
	add			t1,a3,x0
	add			a0,t3,x0
.long_func_640:
	add			t3,t1,x0
	add			t1,a0,x0
	jal			x0,.long_func_637
.long_func_638:
	add			t1,t2,x0
	lui			t3,%hi(SHIFT_TABLE)
	addi		t3,t3,%lo(SHIFT_TABLE)
	slli		t1,t1,2
	add			t1,t3,t1
	lw			t1,0(t1)
	divw		t1,a3,t1
	lui			t3,16
	addw		t3,t1,t3
	addiw		a0,x0,15
	subw		a0,a0,t2
	addiw		a1,x0,1
	addw		a0,a0,a1
	lui			a1,%hi(SHIFT_TABLE)
	addi		a1,a1,%lo(SHIFT_TABLE)
	slli		a0,a0,2
	add			a0,a1,a0
	lw			a0,0(a0)
	subw		t3,t3,a0
	add			a0,t3,x0
	jal			x0,.long_func_640
.long_func_629:
	addiw		t1,x0,0
	blt			a3,t1,.long_func_632
.long_func_633:
	addiw		t1,x0,0
.long_func_634:
	add			t3,a3,x0
	jal			x0,.long_func_631
.long_func_632:
	lui			t1,16
	addiw		t1,t1,-1
	jal			x0,.long_func_634
.long_func_602:
	addiw		t1,x0,0
	addiw		t2,x0,0
	add			t3,a0,x0
	add			t6,a4,x0
	add			a1,t2,x0
.long_func_604:
	addiw		t2,x0,16
	blt			a1,t2,.long_func_605
.long_func_606:
	addiw		t2,x0,0
	addiw		t3,x0,0
	add			t6,a0,x0
	add			a0,a4,x0
	add			a1,t3,x0
.long_func_614:
	addiw		t3,x0,16
	blt			a1,t3,.long_func_615
.long_func_616:
	addiw		t3,x0,1
	addiw		a4,x0,15
	bgt			t3,a4,.long_func_620
.long_func_621:
	addiw		t3,x0,0
	addiw		t6,x0,0
	addiw		a0,x0,1
	lui			a1,%hi(SHIFT_TABLE)
	addi		a1,a1,%lo(SHIFT_TABLE)
	slli		a0,a0,2
	add			a0,a1,a0
	lw			a0,0(a0)
	mulw		t2,t2,a0
	lui			a0,16
	addiw		a0,a0,-1
	add			a1,t2,x0
	add			t2,t3,x0
.long_func_623:
	addiw		t3,x0,16
	blt			t6,t3,.long_func_624
.long_func_625:
	add			t3,a0,x0
	add			a5,a1,x0
.long_func_622:
	add			a4,t1,x0
	add			a0,t1,x0
	add			a0,t2,x0
	add			a1,t3,x0
	add			t3,a5,x0
	add			t1,t2,x0
	jal			x0,.long_func_601
.long_func_624:
	addiw		t3,x0,2
	srliw		t3,a1,31
	add			t3,t3,a1
	andi		t3,t3,-2
	subw		t3,a1,t3
	bne			t3,x0,.long_func_628
.long_func_849:
	add			a4,t2,x0
.long_func_627:
	addiw		t2,x0,2
	srliw		t2,a1,31
	add			t2,a1,t2
	sraiw		t3,t2,1
	addiw		t2,x0,2
	srliw		t2,a0,31
	add			t2,a0,t2
	sraiw		t2,t2,1
	addiw		a0,x0,1
	addw		t6,t6,a0
	add			a0,t2,x0
	add			a1,t3,x0
	add			t2,a4,x0
	jal			x0,.long_func_623
.long_func_628:
	addiw		t3,x0,2
	srliw		t3,a0,31
	add			t3,t3,a0
	andi		t3,t3,-2
	subw		t3,a0,t3
	bne			t3,x0,.long_func_626
.long_func_850:
	add			a4,t2,x0
	jal			x0,.long_func_627
.long_func_626:
	addiw		t3,x0,1
	add			t3,t6,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t3,t3,2
	add			t3,a4,t3
	lw			t3,0(t3)
	addw		t2,t2,t3
	add			a4,t2,x0
	jal			x0,.long_func_627
.long_func_620:
	addiw		t2,x0,0
	add			t3,t6,x0
	add			a5,a0,x0
	add			t6,a1,x0
	jal			x0,.long_func_622
.long_func_615:
	addiw		t3,x0,2
	srliw		t3,a0,31
	add			t3,t3,a0
	andi		t3,t3,-2
	subw		t3,a0,t3
	bne			t3,x0,.long_func_619
.long_func_847:
	add			a4,t2,x0
.long_func_618:
	addiw		t2,x0,2
	srliw		t2,a0,31
	add			t2,a0,t2
	sraiw		t3,t2,1
	addiw		t2,x0,2
	srliw		t2,t6,31
	add			t2,t6,t2
	sraiw		t2,t2,1
	addiw		t6,x0,1
	addw		a1,a1,t6
	add			t6,t2,x0
	add			a0,t3,x0
	add			t2,a4,x0
	jal			x0,.long_func_614
.long_func_619:
	addiw		t3,x0,2
	srliw		t3,t6,31
	add			t3,t3,t6
	andi		t3,t3,-2
	subw		t3,t6,t3
	bne			t3,x0,.long_func_617
.long_func_848:
	add			a4,t2,x0
	jal			x0,.long_func_618
.long_func_617:
	addiw		t3,x0,1
	add			t3,a1,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t3,t3,2
	add			t3,a4,t3
	lw			t3,0(t3)
	addw		t2,t2,t3
	add			a4,t2,x0
	jal			x0,.long_func_618
.long_func_605:
	addiw		t2,x0,2
	srliw		t2,t6,31
	add			t2,t2,t6
	andi		t2,t2,-2
	subw		t2,t6,t2
	bne			t2,x0,.long_func_607
.long_func_608:
	addiw		t2,x0,2
	srliw		t2,t3,31
	add			t2,t2,t3
	andi		t2,t2,-2
	subw		t2,t3,t2
	bne			t2,x0,.long_func_612
.long_func_846:
.long_func_613:
.long_func_609:
	addiw		t2,x0,2
	srliw		t2,t6,31
	add			t2,t6,t2
	sraiw		t6,t2,1
	addiw		t2,x0,2
	srliw		t2,t3,31
	add			t2,t3,t2
	sraiw		t2,t2,1
	addiw		t3,x0,1
	addw		a1,a1,t3
	add			t3,t2,x0
	jal			x0,.long_func_604
.long_func_612:
	addiw		t2,x0,1
	add			t2,a1,x0
	lui			a5,%hi(SHIFT_TABLE)
	addi		a5,a5,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,a5,t2
	lw			t2,0(t2)
	addw		t1,t1,t2
	jal			x0,.long_func_613
.long_func_607:
	addiw		t2,x0,2
	srliw		t2,t3,31
	add			t2,t2,t3
	andi		t2,t2,-2
	subw		t2,t3,t2
	addiw		a5,x0,0
	beq			t2,a5,.long_func_610
.long_func_845:
.long_func_611:
	jal			x0,.long_func_609
.long_func_610:
	addiw		t2,x0,1
	add			t2,a1,x0
	lui			a5,%hi(SHIFT_TABLE)
	addi		a5,a5,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,a5,t2
	lw			t2,0(t2)
	addw		t1,t1,t2
	jal			x0,.long_func_611
.long_func_571:
	addiw		t0,x0,0
	add			a4,a1,x0
	add			t0,t1,x0
.long_func_573:
	bne			a4,x0,.long_func_574
.long_func_575:
	add			t0,a0,x0
	add			t1,a0,x0
	jal			x0,.long_func_572
.long_func_574:
	addiw		t0,x0,0
	addiw		t1,x0,0
	add			t2,a4,x0
	add			t3,a0,x0
	add			t6,t1,x0
.long_func_576:
	addiw		t1,x0,16
	blt			t6,t1,.long_func_577
.long_func_578:
	addiw		t1,x0,0
	addiw		t2,x0,0
	add			t3,a4,x0
	add			t6,a0,x0
	add			a0,t2,x0
.long_func_586:
	addiw		t2,x0,16
	blt			a0,t2,.long_func_587
.long_func_588:
	addiw		t2,x0,1
	addiw		a4,x0,15
	bgt			t2,a4,.long_func_592
.long_func_593:
	addiw		t2,x0,0
	addiw		t3,x0,0
	addiw		t6,x0,1
	lui			a0,%hi(SHIFT_TABLE)
	addi		a0,a0,%lo(SHIFT_TABLE)
	slli		t6,t6,2
	add			t6,a0,t6
	lw			t6,0(t6)
	mulw		t1,t1,t6
	lui			t6,16
	addiw		t6,t6,-1
	add			a0,t1,x0
	add			a4,t3,x0
	add			t1,t2,x0
.long_func_595:
	addiw		t2,x0,16
	blt			a4,t2,.long_func_596
.long_func_597:
	add			t2,t6,x0
	add			t3,a0,x0
	add			t6,a4,x0
.long_func_594:
	add			a0,t0,x0
	add			a4,t1,x0
	add			t0,t1,x0
	jal			x0,.long_func_573
.long_func_596:
	addiw		t2,x0,2
	srliw		t2,a0,31
	add			t2,t2,a0
	andi		t2,t2,-2
	subw		t2,a0,t2
	bne			t2,x0,.long_func_600
.long_func_843:
	add			a5,t1,x0
.long_func_599:
	addiw		t1,x0,2
	srliw		t1,a0,31
	add			t1,a0,t1
	sraiw		t2,t1,1
	addiw		t1,x0,2
	srliw		t1,t6,31
	add			t1,t6,t1
	sraiw		t1,t1,1
	addiw		t3,x0,1
	addw		t3,a4,t3
	add			t6,t1,x0
	add			a0,t2,x0
	add			a4,t3,x0
	add			t1,a5,x0
	jal			x0,.long_func_595
.long_func_600:
	addiw		t2,x0,2
	srliw		t2,t6,31
	add			t2,t2,t6
	andi		t2,t2,-2
	subw		t2,t6,t2
	bne			t2,x0,.long_func_598
.long_func_844:
	add			a5,t1,x0
	jal			x0,.long_func_599
.long_func_598:
	addiw		t2,x0,1
	add			t2,a4,x0
	lui			t3,%hi(SHIFT_TABLE)
	addi		t3,t3,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,t3,t2
	lw			t2,0(t2)
	addw		t1,t1,t2
	add			a5,t1,x0
	jal			x0,.long_func_599
.long_func_592:
	addiw		t1,x0,0
	add			t2,t3,x0
	add			t3,t6,x0
	add			t6,a0,x0
	jal			x0,.long_func_594
.long_func_587:
	addiw		t2,x0,2
	srliw		t2,t6,31
	add			t2,t2,t6
	andi		t2,t2,-2
	subw		t2,t6,t2
	bne			t2,x0,.long_func_591
.long_func_841:
	add			a4,t1,x0
.long_func_590:
	addiw		t1,x0,2
	srliw		t1,t6,31
	add			t1,t6,t1
	sraiw		t2,t1,1
	addiw		t1,x0,2
	srliw		t1,t3,31
	add			t1,t3,t1
	sraiw		t1,t1,1
	addiw		t3,x0,1
	addw		a0,a0,t3
	add			t3,t1,x0
	add			t6,t2,x0
	add			t1,a4,x0
	jal			x0,.long_func_586
.long_func_591:
	addiw		t2,x0,2
	srliw		t2,t3,31
	add			t2,t2,t3
	andi		t2,t2,-2
	subw		t2,t3,t2
	bne			t2,x0,.long_func_589
.long_func_842:
	add			a4,t1,x0
	jal			x0,.long_func_590
.long_func_589:
	addiw		t2,x0,1
	add			t2,a0,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,a4,t2
	lw			t2,0(t2)
	addw		t1,t1,t2
	add			a4,t1,x0
	jal			x0,.long_func_590
.long_func_577:
	addiw		t1,x0,2
	srliw		t1,t3,31
	add			t1,t1,t3
	andi		t1,t1,-2
	subw		t1,t3,t1
	bne			t1,x0,.long_func_579
.long_func_580:
	addiw		t1,x0,2
	srliw		t1,t2,31
	add			t1,t1,t2
	andi		t1,t1,-2
	subw		t1,t2,t1
	bne			t1,x0,.long_func_584
.long_func_840:
.long_func_585:
.long_func_581:
	addiw		t1,x0,2
	srliw		t1,t3,31
	add			t1,t3,t1
	sraiw		t3,t1,1
	addiw		t1,x0,2
	srliw		t1,t2,31
	add			t1,t2,t1
	sraiw		t1,t1,1
	addiw		t2,x0,1
	addw		t6,t6,t2
	add			t2,t1,x0
	jal			x0,.long_func_576
.long_func_584:
	addiw		t1,x0,1
	add			t1,t6,x0
	lui			a5,%hi(SHIFT_TABLE)
	addi		a5,a5,%lo(SHIFT_TABLE)
	slli		t1,t1,2
	add			t1,a5,t1
	lw			t1,0(t1)
	addw		t0,t0,t1
	jal			x0,.long_func_585
.long_func_579:
	addiw		t1,x0,2
	srliw		t1,t2,31
	add			t1,t1,t2
	andi		t1,t1,-2
	subw		t1,t2,t1
	addiw		a5,x0,0
	beq			t1,a5,.long_func_582
.long_func_839:
.long_func_583:
	jal			x0,.long_func_581
.long_func_582:
	addiw		t1,x0,1
	add			t1,t6,x0
	lui			a5,%hi(SHIFT_TABLE)
	addi		a5,a5,%lo(SHIFT_TABLE)
	slli		t1,t1,2
	add			t1,a5,t1
	lw			t1,0(t1)
	addw		t0,t0,t1
	jal			x0,.long_func_583
.long_func_566:
	addiw		t0,x0,2
	srliw		t0,t3,31
	add			t0,t0,t3
	andi		t0,t0,-2
	subw		t0,t3,t0
	bne			t0,x0,.long_func_570
.long_func_836:
	add			a4,t1,x0
.long_func_569:
	addiw		t0,x0,2
	srliw		t0,t3,31
	add			t0,t3,t0
	sraiw		t1,t0,1
	addiw		t0,x0,2
	srliw		t0,t2,31
	add			t0,t2,t0
	sraiw		t0,t0,1
	addiw		t2,x0,1
	addw		t6,t6,t2
	add			t2,t0,x0
	add			t3,t1,x0
	add			t1,a4,x0
	jal			x0,.long_func_565
.long_func_570:
	addiw		t0,x0,2
	srliw		t0,t2,31
	add			t0,t0,t2
	andi		t0,t0,-2
	subw		t0,t2,t0
	bne			t0,x0,.long_func_568
.long_func_837:
	add			a4,t1,x0
	jal			x0,.long_func_569
.long_func_568:
	addiw		t0,x0,1
	add			t0,t6,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t0,t0,2
	add			t0,a4,t0
	lw			t0,0(t0)
	addw		t0,t1,t0
	add			a4,t0,x0
	jal			x0,.long_func_569
.long_func_555:
	addiw		t0,x0,2
	srliw		t0,t3,31
	add			t0,t0,t3
	andi		t0,t0,-2
	subw		t0,t3,t0
	bne			t0,x0,.long_func_559
.long_func_833:
	add			a0,t1,x0
.long_func_558:
	addiw		t0,x0,2
	srliw		t0,t3,31
	add			t0,t3,t0
	sraiw		t1,t0,1
	addiw		t0,x0,2
	srliw		t0,t2,31
	add			t0,t2,t0
	sraiw		t0,t0,1
	addiw		t2,x0,1
	addw		t6,t6,t2
	add			t2,t0,x0
	add			t3,t1,x0
	add			t1,a0,x0
	jal			x0,.long_func_554
.long_func_559:
	addiw		t0,x0,2
	srliw		t0,t2,31
	add			t0,t0,t2
	andi		t0,t0,-2
	subw		t0,t2,t0
	bne			t0,x0,.long_func_557
.long_func_834:
	add			a0,t1,x0
	jal			x0,.long_func_558
.long_func_557:
	addiw		t0,x0,1
	add			t0,t6,x0
	lui			a0,%hi(SHIFT_TABLE)
	addi		a0,a0,%lo(SHIFT_TABLE)
	slli		t0,t0,2
	add			t0,a0,t0
	lw			t0,0(t0)
	addw		t0,t1,t0
	add			a0,t0,x0
	jal			x0,.long_func_558
.long_func_365:
	addiw		t4,x0,2
	addiw		t5,x0,1
	add			s1,t0,x0
	add			s2,t1,x0
	add			s3,t2,x0
	add			s4,t5,x0
	add			t1,fp,x0
	add			t0,t3,x0
.long_func_367:
	addiw		t0,x0,0
	bgt			t1,t0,.long_func_368
.long_func_369:
	add			a0,s4,x0
	call		putint
	addiw		t0,x0,10
	add			a0,t0,x0
	call		putch
	addiw		t0,x0,1
	addw		t0,fp,t0
	add			fp,t0,x0
	add			t0,s1,x0
	add			t1,s2,x0
	add			t2,s3,x0
	add			t3,s4,x0
	jal			x0,.long_func_364
.long_func_368:
	addiw		t0,x0,0
	addiw		t2,x0,0
	addiw		t3,x0,1
	add			t5,t1,x0
	add			t6,t2,x0
	add			t2,t0,x0
.long_func_370:
	addiw		t0,x0,16
	blt			t6,t0,.long_func_371
.long_func_372:
	bne			t2,x0,.long_func_376
.long_func_802:
	add			t0,s4,x0
	add			a1,t3,x0
	add			a2,t5,x0
	add			t5,t6,x0
.long_func_377:
	addiw		t3,x0,0
	add			t6,t4,x0
	add			a0,t4,x0
	add			a1,a2,x0
.long_func_457:
	bne			t6,x0,.long_func_458
.long_func_459:
	addiw		t4,x0,1
	addiw		t2,x0,15
	bge			t4,t2,.long_func_536
.long_func_537:
	addiw		t2,x0,0
	bgt			t4,t2,.long_func_542
.long_func_543:
	add			t6,t1,x0
.long_func_544:
	add			t2,t1,x0
	add			t1,t6,x0
.long_func_538:
	add			s1,t4,x0
	add			s2,t1,x0
	add			s3,t5,x0
	add			s4,t0,x0
	add			t1,t2,x0
	add			t4,t3,x0
	add			t0,t2,x0
	jal			x0,.long_func_367
.long_func_542:
	lui			t2,8
	addiw		t2,t2,-1
	bgt			t1,t2,.long_func_545
.long_func_546:
	add			t2,t4,x0
	lui			t6,%hi(SHIFT_TABLE)
	addi		t6,t6,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,t6,t2
	lw			t2,0(t2)
	divw		t2,t1,t2
.long_func_547:
	add			t6,t1,x0
	add			t1,t2,x0
	jal			x0,.long_func_544
.long_func_545:
	add			t2,t4,x0
	lui			t6,%hi(SHIFT_TABLE)
	addi		t6,t6,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,t6,t2
	lw			t2,0(t2)
	divw		t1,t1,t2
	lui			t2,16
	addw		t2,t1,t2
	addiw		t6,x0,15
	subw		t6,t6,t4
	addiw		a0,x0,1
	addw		t6,t6,a0
	lui			a0,%hi(SHIFT_TABLE)
	addi		a0,a0,%lo(SHIFT_TABLE)
	slli		t6,t6,2
	add			t6,a0,t6
	lw			t6,0(t6)
	subw		t2,t2,t6
	jal			x0,.long_func_547
.long_func_536:
	addiw		t2,x0,0
	blt			t1,t2,.long_func_539
.long_func_540:
	addiw		t2,x0,0
.long_func_541:
	jal			x0,.long_func_538
.long_func_539:
	lui			t2,16
	addiw		t2,t2,-1
	jal			x0,.long_func_541
.long_func_458:
	addiw		t2,x0,0
	addiw		t4,x0,0
	addiw		t5,x0,1
	add			a1,t6,x0
	add			a2,t4,x0
	add			t4,t2,x0
.long_func_460:
	addiw		t2,x0,16
	blt			a2,t2,.long_func_461
.long_func_462:
	bne			t4,x0,.long_func_466
.long_func_820:
	add			t2,t3,x0
	add			a3,t5,x0
	add			a4,a1,x0
	add			t5,a2,x0
	add			t3,t4,x0
.long_func_467:
	addiw		t4,x0,0
	add			a1,a0,x0
	add			t4,a0,x0
	add			a2,a3,x0
	add			a2,a4,x0
.long_func_496:
	bne			t4,x0,.long_func_497
.long_func_498:
	addiw		a2,x0,1
	addiw		t3,x0,15
	bge			a2,t3,.long_func_524
.long_func_525:
	addiw		t3,x0,0
	bgt			a2,t3,.long_func_530
.long_func_531:
	add			t3,t6,x0
	add			t4,t6,x0
.long_func_532:
	add			a3,t3,x0
.long_func_526:
	add			t3,t2,x0
	add			t6,t4,x0
	add			a0,a1,x0
	add			a1,a2,x0
	add			a1,a3,x0
	add			t2,t4,x0
	jal			x0,.long_func_457
.long_func_530:
	lui			t3,8
	addiw		t3,t3,-1
	bgt			t6,t3,.long_func_533
.long_func_534:
	add			t3,a2,x0
	lui			t4,%hi(SHIFT_TABLE)
	addi		t4,t4,%lo(SHIFT_TABLE)
	slli		t3,t3,2
	add			t3,t4,t3
	lw			t3,0(t3)
	divw		t4,t6,t3
	add			t3,t6,x0
.long_func_535:
	jal			x0,.long_func_532
.long_func_533:
	add			t3,a2,x0
	lui			t4,%hi(SHIFT_TABLE)
	addi		t4,t4,%lo(SHIFT_TABLE)
	slli		t3,t3,2
	add			t3,t4,t3
	lw			t3,0(t3)
	divw		t3,t6,t3
	lui			t4,16
	addw		t4,t3,t4
	addiw		t6,x0,15
	subw		t6,t6,a2
	addiw		a0,x0,1
	addw		t6,t6,a0
	lui			a0,%hi(SHIFT_TABLE)
	addi		a0,a0,%lo(SHIFT_TABLE)
	slli		t6,t6,2
	add			t6,a0,t6
	lw			t6,0(t6)
	subw		t4,t4,t6
	jal			x0,.long_func_535
.long_func_524:
	addiw		t3,x0,0
	blt			t6,t3,.long_func_527
.long_func_528:
	addiw		t3,x0,0
.long_func_529:
	add			t4,t3,x0
	add			a3,t6,x0
	jal			x0,.long_func_526
.long_func_527:
	lui			t3,16
	addiw		t3,t3,-1
	jal			x0,.long_func_529
.long_func_497:
	addiw		t3,x0,0
	addiw		t5,x0,0
	add			a0,t4,x0
	add			a2,a1,x0
	add			a3,t5,x0
.long_func_499:
	addiw		t5,x0,16
	blt			a3,t5,.long_func_500
.long_func_501:
	addiw		t5,x0,0
	addiw		a0,x0,0
	add			a4,a0,x0
.long_func_509:
	addiw		a0,x0,16
	blt			a4,a0,.long_func_510
.long_func_511:
	addiw		a0,x0,1
	addiw		a2,x0,15
	bgt			a0,a2,.long_func_515
.long_func_516:
	addiw		t4,x0,0
	addiw		a0,x0,0
	addiw		a1,x0,1
	lui			a2,%hi(SHIFT_TABLE)
	addi		a2,a2,%lo(SHIFT_TABLE)
	slli		a1,a1,2
	add			a1,a2,a1
	lw			a1,0(a1)
	mulw		t5,t5,a1
	lui			a1,16
	addiw		a1,a1,-1
	add			a4,a0,x0
.long_func_518:
	addiw		a0,x0,16
	blt			a4,a0,.long_func_519
.long_func_520:
	add			a0,t4,x0
	add			a2,a1,x0
	add			a3,t5,x0
	add			t4,a4,x0
.long_func_517:
	add			a1,t3,x0
	add			t5,t4,x0
	add			t4,t3,x0
	add			t4,a0,x0
	add			a2,a3,x0
	add			t3,a0,x0
	jal			x0,.long_func_496
.long_func_519:
	addiw		a0,x0,2
	srliw		a0,t5,31
	add			a0,a0,t5
	andi		a0,a0,-2
	subw		a0,t5,a0
	bne			a0,x0,.long_func_523
.long_func_831:
	add			a2,t4,x0
.long_func_522:
	addiw		t4,x0,2
	srliw		t4,t5,31
	add			t4,t5,t4
	sraiw		t5,t4,1
	addiw		t4,x0,2
	srliw		t4,a1,31
	add			t4,a1,t4
	sraiw		t4,t4,1
	addiw		a0,x0,1
	addw		a0,a4,a0
	add			a1,t4,x0
	add			a4,a0,x0
	add			t4,a2,x0
	jal			x0,.long_func_518
.long_func_523:
	addiw		a0,x0,2
	srliw		a0,a1,31
	add			a0,a0,a1
	andi		a0,a0,-2
	subw		a0,a1,a0
	bne			a0,x0,.long_func_521
.long_func_832:
	add			a2,t4,x0
	jal			x0,.long_func_522
.long_func_521:
	addiw		a0,x0,1
	add			a0,a4,x0
	lui			a2,%hi(SHIFT_TABLE)
	addi		a2,a2,%lo(SHIFT_TABLE)
	slli		a0,a0,2
	add			a0,a2,a0
	lw			a0,0(a0)
	addw		t4,t4,a0
	add			a2,t4,x0
	jal			x0,.long_func_522
.long_func_515:
	addiw		t5,x0,0
	add			a0,t5,x0
	add			a2,t4,x0
	add			a3,a1,x0
	add			t4,a4,x0
	jal			x0,.long_func_517
.long_func_510:
	addiw		a0,x0,2
	srliw		a0,a1,31
	add			a0,a0,a1
	andi		a0,a0,-2
	subw		a0,a1,a0
	bne			a0,x0,.long_func_514
.long_func_829:
	add			a2,t5,x0
.long_func_513:
	addiw		t5,x0,2
	srliw		t5,a1,31
	add			t5,a1,t5
	sraiw		t5,t5,1
	addiw		a0,x0,2
	srliw		a0,t4,31
	add			t4,t4,a0
	sraiw		t4,t4,1
	addiw		a0,x0,1
	addw		a0,a4,a0
	add			a1,t5,x0
	add			a4,a0,x0
	add			t5,a2,x0
	jal			x0,.long_func_509
.long_func_514:
	addiw		a0,x0,2
	srliw		a0,t4,31
	add			a0,a0,t4
	andi		a0,a0,-2
	subw		a0,t4,a0
	bne			a0,x0,.long_func_512
.long_func_830:
	add			a2,t5,x0
	jal			x0,.long_func_513
.long_func_512:
	addiw		a0,x0,1
	add			a0,a4,x0
	lui			a2,%hi(SHIFT_TABLE)
	addi		a2,a2,%lo(SHIFT_TABLE)
	slli		a0,a0,2
	add			a0,a2,a0
	lw			a0,0(a0)
	addw		t5,t5,a0
	add			a2,t5,x0
	jal			x0,.long_func_513
.long_func_500:
	addiw		t5,x0,2
	srliw		t5,a2,31
	add			t5,t5,a2
	andi		t5,t5,-2
	subw		t5,a2,t5
	bne			t5,x0,.long_func_502
.long_func_503:
	addiw		t5,x0,2
	srliw		t5,a0,31
	add			t5,t5,a0
	andi		t5,t5,-2
	subw		t5,a0,t5
	bne			t5,x0,.long_func_507
.long_func_828:
.long_func_508:
.long_func_504:
	addiw		t5,x0,2
	srliw		t5,a2,31
	add			t5,a2,t5
	sraiw		a2,t5,1
	addiw		t5,x0,2
	srliw		t5,a0,31
	add			t5,a0,t5
	sraiw		t5,t5,1
	addiw		a0,x0,1
	addw		a3,a3,a0
	add			a0,t5,x0
	jal			x0,.long_func_499
.long_func_507:
	addiw		t5,x0,1
	add			t5,a3,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t5,t5,2
	add			t5,a4,t5
	lw			t5,0(t5)
	addw		t3,t3,t5
	jal			x0,.long_func_508
.long_func_502:
	addiw		t5,x0,2
	srliw		t5,a0,31
	add			t5,t5,a0
	andi		t5,t5,-2
	subw		t5,a0,t5
	addiw		a4,x0,0
	beq			t5,a4,.long_func_505
.long_func_827:
.long_func_506:
	jal			x0,.long_func_504
.long_func_505:
	addiw		t5,x0,1
	add			t5,a3,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t5,t5,2
	add			t5,a4,t5
	lw			t5,0(t5)
	addw		t3,t3,t5
	jal			x0,.long_func_506
.long_func_466:
	addiw		t2,x0,0
	add			a3,a0,x0
	add			t2,t4,x0
.long_func_468:
	bne			a3,x0,.long_func_469
.long_func_470:
	add			t2,t3,x0
	add			a3,t5,x0
	add			a4,a1,x0
	add			t5,a2,x0
	jal			x0,.long_func_467
.long_func_469:
	addiw		t2,x0,0
	addiw		t4,x0,0
	add			t5,a3,x0
	add			a1,t3,x0
	add			a2,t4,x0
.long_func_471:
	addiw		t4,x0,16
	blt			a2,t4,.long_func_472
.long_func_473:
	addiw		t4,x0,0
	addiw		t5,x0,0
	add			a1,a3,x0
	add			a2,t5,x0
.long_func_481:
	addiw		t5,x0,16
	blt			a2,t5,.long_func_482
.long_func_483:
	addiw		t5,x0,1
	addiw		a3,x0,15
	bgt			t5,a3,.long_func_487
.long_func_488:
	addiw		t3,x0,0
	addiw		t5,x0,0
	addiw		a1,x0,1
	lui			a2,%hi(SHIFT_TABLE)
	addi		a2,a2,%lo(SHIFT_TABLE)
	slli		a1,a1,2
	add			a1,a2,a1
	lw			a1,0(a1)
	mulw		t4,t4,a1
	lui			a1,16
	addiw		a1,a1,-1
	add			a2,t4,x0
	add			a3,t5,x0
.long_func_490:
	addiw		t4,x0,16
	blt			a3,t4,.long_func_491
.long_func_492:
	add			t4,t3,x0
	add			t5,a1,x0
	add			a1,a2,x0
	add			a2,a3,x0
.long_func_489:
	add			t3,t2,x0
	add			a3,t4,x0
	add			t2,t4,x0
	jal			x0,.long_func_468
.long_func_491:
	addiw		t4,x0,2
	srliw		t4,a2,31
	add			t4,t4,a2
	andi		t4,t4,-2
	subw		t4,a2,t4
	bne			t4,x0,.long_func_495
.long_func_825:
	add			a4,t3,x0
.long_func_494:
	addiw		t3,x0,2
	srliw		t3,a2,31
	add			t3,a2,t3
	sraiw		t4,t3,1
	addiw		t3,x0,2
	srliw		t3,a1,31
	add			t3,a1,t3
	sraiw		t3,t3,1
	addiw		t5,x0,1
	addw		t5,a3,t5
	add			a1,t3,x0
	add			a2,t4,x0
	add			a3,t5,x0
	add			t3,a4,x0
	jal			x0,.long_func_490
.long_func_495:
	addiw		t4,x0,2
	srliw		t4,a1,31
	add			t4,t4,a1
	andi		t4,t4,-2
	subw		t4,a1,t4
	bne			t4,x0,.long_func_493
.long_func_826:
	add			a4,t3,x0
	jal			x0,.long_func_494
.long_func_493:
	addiw		t4,x0,1
	add			t4,a3,x0
	lui			t5,%hi(SHIFT_TABLE)
	addi		t5,t5,%lo(SHIFT_TABLE)
	slli		t4,t4,2
	add			t4,t5,t4
	lw			t4,0(t4)
	addw		t3,t3,t4
	add			a4,t3,x0
	jal			x0,.long_func_494
.long_func_487:
	addiw		t4,x0,0
	add			t5,a1,x0
	add			a1,t3,x0
	jal			x0,.long_func_489
.long_func_482:
	addiw		t5,x0,2
	srliw		t5,t3,31
	add			t5,t5,t3
	andi		t5,t5,-2
	subw		t5,t3,t5
	bne			t5,x0,.long_func_486
.long_func_823:
	add			a3,t4,x0
.long_func_485:
	addiw		t4,x0,2
	srliw		t4,t3,31
	add			t3,t3,t4
	sraiw		t4,t3,1
	addiw		t3,x0,2
	srliw		t3,a1,31
	add			t3,a1,t3
	sraiw		t3,t3,1
	addiw		t5,x0,1
	addw		t5,a2,t5
	add			a1,t3,x0
	add			t3,t4,x0
	add			a2,t5,x0
	add			t4,a3,x0
	jal			x0,.long_func_481
.long_func_486:
	addiw		t5,x0,2
	srliw		t5,a1,31
	add			t5,t5,a1
	andi		t5,t5,-2
	subw		t5,a1,t5
	bne			t5,x0,.long_func_484
.long_func_824:
	add			a3,t4,x0
	jal			x0,.long_func_485
.long_func_484:
	addiw		t5,x0,1
	add			t5,a2,x0
	lui			a3,%hi(SHIFT_TABLE)
	addi		a3,a3,%lo(SHIFT_TABLE)
	slli		t5,t5,2
	add			t5,a3,t5
	lw			t5,0(t5)
	addw		t4,t4,t5
	add			a3,t4,x0
	jal			x0,.long_func_485
.long_func_472:
	addiw		t4,x0,2
	srliw		t4,a1,31
	add			t4,t4,a1
	andi		t4,t4,-2
	subw		t4,a1,t4
	bne			t4,x0,.long_func_474
.long_func_475:
	addiw		t4,x0,2
	srliw		t4,t5,31
	add			t4,t4,t5
	andi		t4,t4,-2
	subw		t4,t5,t4
	bne			t4,x0,.long_func_479
.long_func_822:
.long_func_480:
.long_func_476:
	addiw		t4,x0,2
	srliw		t4,a1,31
	add			t4,a1,t4
	sraiw		a1,t4,1
	addiw		t4,x0,2
	srliw		t4,t5,31
	add			t4,t5,t4
	sraiw		t4,t4,1
	addiw		t5,x0,1
	addw		a2,a2,t5
	add			t5,t4,x0
	jal			x0,.long_func_471
.long_func_479:
	addiw		t4,x0,1
	add			t4,a2,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t4,t4,2
	add			t4,a4,t4
	lw			t4,0(t4)
	addw		t2,t2,t4
	jal			x0,.long_func_480
.long_func_474:
	addiw		t4,x0,2
	srliw		t4,t5,31
	add			t4,t4,t5
	andi		t4,t4,-2
	subw		t4,t5,t4
	addiw		a4,x0,0
	beq			t4,a4,.long_func_477
.long_func_821:
.long_func_478:
	jal			x0,.long_func_476
.long_func_477:
	addiw		t4,x0,1
	add			t4,a2,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t4,t4,2
	add			t4,a4,t4
	lw			t4,0(t4)
	addw		t2,t2,t4
	jal			x0,.long_func_478
.long_func_461:
	addiw		t2,x0,2
	srliw		t2,a1,31
	add			t2,t2,a1
	andi		t2,t2,-2
	subw		t2,a1,t2
	bne			t2,x0,.long_func_465
.long_func_818:
	add			a3,t4,x0
.long_func_464:
	addiw		t2,x0,2
	srliw		t2,a1,31
	add			t2,a1,t2
	sraiw		t4,t2,1
	addiw		t2,x0,2
	srliw		t2,t5,31
	add			t2,t5,t2
	sraiw		t2,t2,1
	addiw		t5,x0,1
	addw		a2,a2,t5
	add			t5,t2,x0
	add			a1,t4,x0
	add			t4,a3,x0
	jal			x0,.long_func_460
.long_func_465:
	addiw		t2,x0,2
	srliw		t2,t5,31
	add			t2,t2,t5
	andi		t2,t2,-2
	subw		t2,t5,t2
	bne			t2,x0,.long_func_463
.long_func_819:
	add			a3,t4,x0
	jal			x0,.long_func_464
.long_func_463:
	addiw		t2,x0,1
	add			t2,a2,x0
	lui			a3,%hi(SHIFT_TABLE)
	addi		a3,a3,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,a3,t2
	lw			t2,0(t2)
	addw		t2,t4,t2
	add			a3,t2,x0
	jal			x0,.long_func_464
.long_func_376:
	addiw		t0,x0,0
	add			a0,t0,x0
	add			a1,t4,x0
	add			a2,s4,x0
	add			t0,t2,x0
.long_func_378:
	bne			a1,x0,.long_func_379
.long_func_380:
	add			t0,a0,x0
	add			a1,t3,x0
	add			a2,t5,x0
	add			t5,t6,x0
	add			t2,a0,x0
	jal			x0,.long_func_377
.long_func_379:
	addiw		t0,x0,0
	addiw		t2,x0,0
	addiw		t3,x0,1
	add			t5,a1,x0
	add			t6,t2,x0
	add			t2,t0,x0
.long_func_381:
	addiw		t0,x0,16
	blt			t6,t0,.long_func_382
.long_func_383:
	bne			t2,x0,.long_func_387
.long_func_805:
	add			t0,a0,x0
.long_func_388:
	addiw		a0,x0,0
	add			a3,a2,x0
	add			a0,a2,x0
	add			a2,t3,x0
.long_func_417:
	bne			a0,x0,.long_func_418
.long_func_419:
	addiw		t3,x0,1
	addiw		t2,x0,15
	bge			t3,t2,.long_func_445
.long_func_446:
	addiw		t2,x0,0
	bgt			t3,t2,.long_func_451
.long_func_452:
	add			t5,a1,x0
	add			t2,a1,x0
.long_func_453:
.long_func_447:
	add			a0,t0,x0
	add			a1,t2,x0
	add			a2,a3,x0
	add			t0,t2,x0
	jal			x0,.long_func_378
.long_func_451:
	lui			t2,8
	addiw		t2,t2,-1
	bgt			a1,t2,.long_func_454
.long_func_455:
	add			t2,t3,x0
	lui			t5,%hi(SHIFT_TABLE)
	addi		t5,t5,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,t5,t2
	lw			t2,0(t2)
	divw		t5,a1,t2
	add			t2,a1,x0
	add			a0,t5,x0
.long_func_456:
	add			t5,t2,x0
	add			t2,a0,x0
	jal			x0,.long_func_453
.long_func_454:
	add			t2,t3,x0
	lui			t5,%hi(SHIFT_TABLE)
	addi		t5,t5,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,t5,t2
	lw			t2,0(t2)
	divw		t2,a1,t2
	lui			t5,16
	addw		t5,t2,t5
	addiw		a0,x0,15
	subw		a0,a0,t3
	addiw		a1,x0,1
	addw		a0,a0,a1
	lui			a1,%hi(SHIFT_TABLE)
	addi		a1,a1,%lo(SHIFT_TABLE)
	slli		a0,a0,2
	add			a0,a1,a0
	lw			a0,0(a0)
	subw		t5,t5,a0
	add			a0,t5,x0
	jal			x0,.long_func_456
.long_func_445:
	addiw		t2,x0,0
	blt			a1,t2,.long_func_448
.long_func_449:
	addiw		t2,x0,0
.long_func_450:
	add			t5,a1,x0
	jal			x0,.long_func_447
.long_func_448:
	lui			t2,16
	addiw		t2,t2,-1
	jal			x0,.long_func_450
.long_func_418:
	addiw		t2,x0,0
	addiw		t3,x0,0
	add			t5,a0,x0
	add			t6,a3,x0
	add			a2,t3,x0
.long_func_420:
	addiw		t3,x0,16
	blt			a2,t3,.long_func_421
.long_func_422:
	addiw		t3,x0,0
	addiw		t5,x0,0
	add			t6,a0,x0
	add			a0,a3,x0
	add			a2,t5,x0
.long_func_430:
	addiw		t5,x0,16
	blt			a2,t5,.long_func_431
.long_func_432:
	addiw		t5,x0,1
	addiw		a3,x0,15
	bgt			t5,a3,.long_func_436
.long_func_437:
	addiw		t5,x0,0
	addiw		t6,x0,0
	addiw		a0,x0,1
	lui			a2,%hi(SHIFT_TABLE)
	addi		a2,a2,%lo(SHIFT_TABLE)
	slli		a0,a0,2
	add			a0,a2,a0
	lw			a0,0(a0)
	mulw		t3,t3,a0
	lui			a0,16
	addiw		a0,a0,-1
	add			a2,t3,x0
	add			t3,t5,x0
.long_func_439:
	addiw		t5,x0,16
	blt			t6,t5,.long_func_440
.long_func_441:
	add			t5,a0,x0
	add			a4,a2,x0
.long_func_438:
	add			a3,t2,x0
	add			a0,t2,x0
	add			a0,t3,x0
	add			a2,t5,x0
	add			t5,a4,x0
	add			t2,t3,x0
	jal			x0,.long_func_417
.long_func_440:
	addiw		t5,x0,2
	srliw		t5,a2,31
	add			t5,t5,a2
	andi		t5,t5,-2
	subw		t5,a2,t5
	bne			t5,x0,.long_func_444
.long_func_816:
	add			a3,t3,x0
.long_func_443:
	addiw		t3,x0,2
	srliw		t3,a2,31
	add			t3,a2,t3
	sraiw		t5,t3,1
	addiw		t3,x0,2
	srliw		t3,a0,31
	add			t3,a0,t3
	sraiw		t3,t3,1
	addiw		a0,x0,1
	addw		t6,t6,a0
	add			a0,t3,x0
	add			a2,t5,x0
	add			t3,a3,x0
	jal			x0,.long_func_439
.long_func_444:
	addiw		t5,x0,2
	srliw		t5,a0,31
	add			t5,t5,a0
	andi		t5,t5,-2
	subw		t5,a0,t5
	bne			t5,x0,.long_func_442
.long_func_817:
	add			a3,t3,x0
	jal			x0,.long_func_443
.long_func_442:
	addiw		t5,x0,1
	add			t5,t6,x0
	lui			a3,%hi(SHIFT_TABLE)
	addi		a3,a3,%lo(SHIFT_TABLE)
	slli		t5,t5,2
	add			t5,a3,t5
	lw			t5,0(t5)
	addw		t3,t3,t5
	add			a3,t3,x0
	jal			x0,.long_func_443
.long_func_436:
	addiw		t3,x0,0
	add			t5,t6,x0
	add			a4,a0,x0
	add			t6,a2,x0
	jal			x0,.long_func_438
.long_func_431:
	addiw		t5,x0,2
	srliw		t5,a0,31
	add			t5,t5,a0
	andi		t5,t5,-2
	subw		t5,a0,t5
	bne			t5,x0,.long_func_435
.long_func_814:
	add			a3,t3,x0
.long_func_434:
	addiw		t3,x0,2
	srliw		t3,a0,31
	add			t3,a0,t3
	sraiw		t5,t3,1
	addiw		t3,x0,2
	srliw		t3,t6,31
	add			t3,t6,t3
	sraiw		t3,t3,1
	addiw		t6,x0,1
	addw		a2,a2,t6
	add			t6,t3,x0
	add			a0,t5,x0
	add			t3,a3,x0
	jal			x0,.long_func_430
.long_func_435:
	addiw		t5,x0,2
	srliw		t5,t6,31
	add			t5,t5,t6
	andi		t5,t5,-2
	subw		t5,t6,t5
	bne			t5,x0,.long_func_433
.long_func_815:
	add			a3,t3,x0
	jal			x0,.long_func_434
.long_func_433:
	addiw		t5,x0,1
	add			t5,a2,x0
	lui			a3,%hi(SHIFT_TABLE)
	addi		a3,a3,%lo(SHIFT_TABLE)
	slli		t5,t5,2
	add			t5,a3,t5
	lw			t5,0(t5)
	addw		t3,t3,t5
	add			a3,t3,x0
	jal			x0,.long_func_434
.long_func_421:
	addiw		t3,x0,2
	srliw		t3,t6,31
	add			t3,t3,t6
	andi		t3,t3,-2
	subw		t3,t6,t3
	bne			t3,x0,.long_func_423
.long_func_424:
	addiw		t3,x0,2
	srliw		t3,t5,31
	add			t3,t3,t5
	andi		t3,t3,-2
	subw		t3,t5,t3
	bne			t3,x0,.long_func_428
.long_func_813:
.long_func_429:
.long_func_425:
	addiw		t3,x0,2
	srliw		t3,t6,31
	add			t3,t6,t3
	sraiw		t6,t3,1
	addiw		t3,x0,2
	srliw		t3,t5,31
	add			t3,t5,t3
	sraiw		t3,t3,1
	addiw		t5,x0,1
	addw		a2,a2,t5
	add			t5,t3,x0
	jal			x0,.long_func_420
.long_func_428:
	addiw		t3,x0,1
	add			t3,a2,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t3,t3,2
	add			t3,a4,t3
	lw			t3,0(t3)
	addw		t2,t2,t3
	jal			x0,.long_func_429
.long_func_423:
	addiw		t3,x0,2
	srliw		t3,t5,31
	add			t3,t3,t5
	andi		t3,t3,-2
	subw		t3,t5,t3
	addiw		a4,x0,0
	beq			t3,a4,.long_func_426
.long_func_812:
.long_func_427:
	jal			x0,.long_func_425
.long_func_426:
	addiw		t3,x0,1
	add			t3,a2,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t3,t3,2
	add			t3,a4,t3
	lw			t3,0(t3)
	addw		t2,t2,t3
	jal			x0,.long_func_427
.long_func_387:
	addiw		t0,x0,0
	add			a3,a2,x0
	add			t0,t2,x0
.long_func_389:
	bne			a3,x0,.long_func_390
.long_func_391:
	add			t0,a0,x0
	add			t2,a0,x0
	jal			x0,.long_func_388
.long_func_390:
	addiw		t0,x0,0
	addiw		t2,x0,0
	add			t3,a3,x0
	add			t5,a0,x0
	add			t6,t2,x0
.long_func_392:
	addiw		t2,x0,16
	blt			t6,t2,.long_func_393
.long_func_394:
	addiw		t2,x0,0
	addiw		t3,x0,0
	add			t5,a3,x0
	add			t6,a0,x0
	add			a0,t3,x0
.long_func_402:
	addiw		t3,x0,16
	blt			a0,t3,.long_func_403
.long_func_404:
	addiw		t3,x0,1
	addiw		a3,x0,15
	bgt			t3,a3,.long_func_408
.long_func_409:
	addiw		t3,x0,0
	addiw		t5,x0,0
	addiw		t6,x0,1
	lui			a0,%hi(SHIFT_TABLE)
	addi		a0,a0,%lo(SHIFT_TABLE)
	slli		t6,t6,2
	add			t6,a0,t6
	lw			t6,0(t6)
	mulw		t2,t2,t6
	lui			t6,16
	addiw		t6,t6,-1
	add			a0,t2,x0
	add			a3,t5,x0
	add			t2,t3,x0
.long_func_411:
	addiw		t3,x0,16
	blt			a3,t3,.long_func_412
.long_func_413:
	add			t3,t6,x0
	add			t5,a0,x0
	add			t6,a3,x0
.long_func_410:
	add			a0,t0,x0
	add			a3,t2,x0
	add			t0,t2,x0
	jal			x0,.long_func_389
.long_func_412:
	addiw		t3,x0,2
	srliw		t3,a0,31
	add			t3,t3,a0
	andi		t3,t3,-2
	subw		t3,a0,t3
	bne			t3,x0,.long_func_416
.long_func_810:
	add			a4,t2,x0
.long_func_415:
	addiw		t2,x0,2
	srliw		t2,a0,31
	add			t2,a0,t2
	sraiw		t3,t2,1
	addiw		t2,x0,2
	srliw		t2,t6,31
	add			t2,t6,t2
	sraiw		t2,t2,1
	addiw		t5,x0,1
	addw		t5,a3,t5
	add			t6,t2,x0
	add			a0,t3,x0
	add			a3,t5,x0
	add			t2,a4,x0
	jal			x0,.long_func_411
.long_func_416:
	addiw		t3,x0,2
	srliw		t3,t6,31
	add			t3,t3,t6
	andi		t3,t3,-2
	subw		t3,t6,t3
	bne			t3,x0,.long_func_414
.long_func_811:
	add			a4,t2,x0
	jal			x0,.long_func_415
.long_func_414:
	addiw		t3,x0,1
	add			t3,a3,x0
	lui			t5,%hi(SHIFT_TABLE)
	addi		t5,t5,%lo(SHIFT_TABLE)
	slli		t3,t3,2
	add			t3,t5,t3
	lw			t3,0(t3)
	addw		t2,t2,t3
	add			a4,t2,x0
	jal			x0,.long_func_415
.long_func_408:
	addiw		t2,x0,0
	add			t3,t5,x0
	add			t5,t6,x0
	add			t6,a0,x0
	jal			x0,.long_func_410
.long_func_403:
	addiw		t3,x0,2
	srliw		t3,t6,31
	add			t3,t3,t6
	andi		t3,t3,-2
	subw		t3,t6,t3
	bne			t3,x0,.long_func_407
.long_func_808:
	add			a3,t2,x0
.long_func_406:
	addiw		t2,x0,2
	srliw		t2,t6,31
	add			t2,t6,t2
	sraiw		t3,t2,1
	addiw		t2,x0,2
	srliw		t2,t5,31
	add			t2,t5,t2
	sraiw		t2,t2,1
	addiw		t5,x0,1
	addw		a0,a0,t5
	add			t5,t2,x0
	add			t6,t3,x0
	add			t2,a3,x0
	jal			x0,.long_func_402
.long_func_407:
	addiw		t3,x0,2
	srliw		t3,t5,31
	add			t3,t3,t5
	andi		t3,t3,-2
	subw		t3,t5,t3
	bne			t3,x0,.long_func_405
.long_func_809:
	add			a3,t2,x0
	jal			x0,.long_func_406
.long_func_405:
	addiw		t3,x0,1
	add			t3,a0,x0
	lui			a3,%hi(SHIFT_TABLE)
	addi		a3,a3,%lo(SHIFT_TABLE)
	slli		t3,t3,2
	add			t3,a3,t3
	lw			t3,0(t3)
	addw		t2,t2,t3
	add			a3,t2,x0
	jal			x0,.long_func_406
.long_func_393:
	addiw		t2,x0,2
	srliw		t2,t5,31
	add			t2,t2,t5
	andi		t2,t2,-2
	subw		t2,t5,t2
	bne			t2,x0,.long_func_395
.long_func_396:
	addiw		t2,x0,2
	srliw		t2,t3,31
	add			t2,t2,t3
	andi		t2,t2,-2
	subw		t2,t3,t2
	bne			t2,x0,.long_func_400
.long_func_807:
.long_func_401:
.long_func_397:
	addiw		t2,x0,2
	srliw		t2,t5,31
	add			t2,t5,t2
	sraiw		t5,t2,1
	addiw		t2,x0,2
	srliw		t2,t3,31
	add			t2,t3,t2
	sraiw		t2,t2,1
	addiw		t3,x0,1
	addw		t6,t6,t3
	add			t3,t2,x0
	jal			x0,.long_func_392
.long_func_400:
	addiw		t2,x0,1
	add			t2,t6,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,a4,t2
	lw			t2,0(t2)
	addw		t0,t0,t2
	jal			x0,.long_func_401
.long_func_395:
	addiw		t2,x0,2
	srliw		t2,t3,31
	add			t2,t2,t3
	andi		t2,t2,-2
	subw		t2,t3,t2
	addiw		a4,x0,0
	beq			t2,a4,.long_func_398
.long_func_806:
.long_func_399:
	jal			x0,.long_func_397
.long_func_398:
	addiw		t2,x0,1
	add			t2,t6,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,a4,t2
	lw			t2,0(t2)
	addw		t0,t0,t2
	jal			x0,.long_func_399
.long_func_382:
	addiw		t0,x0,2
	srliw		t0,t5,31
	add			t0,t0,t5
	andi		t0,t0,-2
	subw		t0,t5,t0
	bne			t0,x0,.long_func_386
.long_func_803:
	add			a3,t2,x0
.long_func_385:
	addiw		t0,x0,2
	srliw		t0,t5,31
	add			t0,t5,t0
	sraiw		t2,t0,1
	addiw		t0,x0,2
	srliw		t0,t3,31
	add			t0,t3,t0
	sraiw		t0,t0,1
	addiw		t3,x0,1
	addw		t6,t6,t3
	add			t3,t0,x0
	add			t5,t2,x0
	add			t2,a3,x0
	jal			x0,.long_func_381
.long_func_386:
	addiw		t0,x0,2
	srliw		t0,t3,31
	add			t0,t0,t3
	andi		t0,t0,-2
	subw		t0,t3,t0
	bne			t0,x0,.long_func_384
.long_func_804:
	add			a3,t2,x0
	jal			x0,.long_func_385
.long_func_384:
	addiw		t0,x0,1
	add			t0,t6,x0
	lui			a3,%hi(SHIFT_TABLE)
	addi		a3,a3,%lo(SHIFT_TABLE)
	slli		t0,t0,2
	add			t0,a3,t0
	lw			t0,0(t0)
	addw		t0,t2,t0
	add			a3,t0,x0
	jal			x0,.long_func_385
.long_func_371:
	addiw		t0,x0,2
	srliw		t0,t5,31
	add			t0,t0,t5
	andi		t0,t0,-2
	subw		t0,t5,t0
	bne			t0,x0,.long_func_375
.long_func_800:
	add			a0,t2,x0
.long_func_374:
	addiw		t0,x0,2
	srliw		t0,t5,31
	add			t0,t5,t0
	sraiw		t2,t0,1
	addiw		t0,x0,2
	srliw		t0,t3,31
	add			t0,t3,t0
	sraiw		t0,t0,1
	addiw		t3,x0,1
	addw		t6,t6,t3
	add			t3,t0,x0
	add			t5,t2,x0
	add			t2,a0,x0
	jal			x0,.long_func_370
.long_func_375:
	addiw		t0,x0,2
	srliw		t0,t3,31
	add			t0,t0,t3
	andi		t0,t0,-2
	subw		t0,t3,t0
	bne			t0,x0,.long_func_373
.long_func_801:
	add			a0,t2,x0
	jal			x0,.long_func_374
.long_func_373:
	addiw		t0,x0,1
	add			t0,t6,x0
	lui			a0,%hi(SHIFT_TABLE)
	addi		a0,a0,%lo(SHIFT_TABLE)
	slli		t0,t0,2
	add			t0,a0,t0
	lw			t0,0(t0)
	addw		t0,t2,t0
	add			a0,t0,x0
	jal			x0,.long_func_374
.long_func_184:
	addiw		t0,x0,0
	addiw		t2,x0,0
	addiw		t4,x0,1
	add			t5,t1,x0
	add			t6,t2,x0
	add			t2,t0,x0
.long_func_186:
	addiw		t0,x0,16
	blt			t6,t0,.long_func_187
.long_func_188:
	bne			t2,x0,.long_func_192
.long_func_769:
	add			t0,s4,x0
	add			a1,t4,x0
.long_func_193:
	addiw		t4,x0,0
	add			a0,t6,x0
	add			t6,t3,x0
	add			a2,t3,x0
.long_func_273:
	bne			t6,x0,.long_func_274
.long_func_275:
	addiw		t5,x0,1
	addiw		t2,x0,15
	bge			t5,t2,.long_func_352
.long_func_353:
	addiw		t2,x0,0
	bgt			t5,t2,.long_func_358
.long_func_359:
	add			t3,t1,x0
.long_func_360:
	add			t2,t1,x0
	add			t6,t3,x0
.long_func_354:
	add			s4,t0,x0
	add			t1,t2,x0
	add			t3,t4,x0
	add			s1,t5,x0
	add			s2,t6,x0
	add			s3,a0,x0
	add			t0,t2,x0
	jal			x0,.long_func_183
.long_func_358:
	lui			t2,8
	addiw		t2,t2,-1
	bgt			t1,t2,.long_func_361
.long_func_362:
	add			t2,t5,x0
	lui			t3,%hi(SHIFT_TABLE)
	addi		t3,t3,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,t3,t2
	lw			t2,0(t2)
	divw		t2,t1,t2
.long_func_363:
	add			t3,t1,x0
	add			t1,t2,x0
	jal			x0,.long_func_360
.long_func_361:
	add			t2,t5,x0
	lui			t3,%hi(SHIFT_TABLE)
	addi		t3,t3,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,t3,t2
	lw			t2,0(t2)
	divw		t1,t1,t2
	lui			t2,16
	addw		t2,t1,t2
	addiw		t3,x0,15
	subw		t3,t3,t5
	addiw		t6,x0,1
	addw		t3,t3,t6
	lui			t6,%hi(SHIFT_TABLE)
	addi		t6,t6,%lo(SHIFT_TABLE)
	slli		t3,t3,2
	add			t3,t6,t3
	lw			t3,0(t3)
	subw		t2,t2,t3
	jal			x0,.long_func_363
.long_func_352:
	addiw		t2,x0,0
	blt			t1,t2,.long_func_355
.long_func_356:
	addiw		t2,x0,0
.long_func_357:
	add			t6,t1,x0
	jal			x0,.long_func_354
.long_func_355:
	lui			t2,16
	addiw		t2,t2,-1
	jal			x0,.long_func_357
.long_func_274:
	addiw		t2,x0,0
	addiw		t3,x0,0
	addiw		t5,x0,1
	add			a0,t6,x0
	add			a1,t3,x0
	add			t3,t2,x0
.long_func_276:
	addiw		t2,x0,16
	blt			a1,t2,.long_func_277
.long_func_278:
	bne			t3,x0,.long_func_282
.long_func_787:
	add			t2,t4,x0
	add			a3,t5,x0
	add			a4,a0,x0
	add			a0,a1,x0
.long_func_283:
	addiw		t4,x0,0
	add			t5,a2,x0
	add			t4,a2,x0
	add			a2,a3,x0
	add			a2,a4,x0
.long_func_312:
	bne			t4,x0,.long_func_313
.long_func_314:
	addiw		a1,x0,1
	addiw		t3,x0,15
	bge			a1,t3,.long_func_340
.long_func_341:
	addiw		t3,x0,0
	bgt			a1,t3,.long_func_346
.long_func_347:
	add			t4,t6,x0
	add			t3,t6,x0
.long_func_348:
	add			a3,t4,x0
.long_func_342:
	add			t4,t2,x0
	add			t6,t3,x0
	add			a2,t5,x0
	add			t5,a3,x0
	add			t2,t3,x0
	jal			x0,.long_func_273
.long_func_346:
	lui			t3,8
	addiw		t3,t3,-1
	bgt			t6,t3,.long_func_349
.long_func_350:
	add			t3,a1,x0
	lui			t4,%hi(SHIFT_TABLE)
	addi		t4,t4,%lo(SHIFT_TABLE)
	slli		t3,t3,2
	add			t3,t4,t3
	lw			t3,0(t3)
	divw		t4,t6,t3
	add			t3,t6,x0
	add			t6,t4,x0
.long_func_351:
	add			t4,t3,x0
	add			t3,t6,x0
	jal			x0,.long_func_348
.long_func_349:
	add			t3,a1,x0
	lui			t4,%hi(SHIFT_TABLE)
	addi		t4,t4,%lo(SHIFT_TABLE)
	slli		t3,t3,2
	add			t3,t4,t3
	lw			t3,0(t3)
	divw		t3,t6,t3
	lui			t4,16
	addw		t4,t3,t4
	addiw		t6,x0,15
	subw		t6,t6,a1
	addiw		a2,x0,1
	addw		t6,t6,a2
	lui			a2,%hi(SHIFT_TABLE)
	addi		a2,a2,%lo(SHIFT_TABLE)
	slli		t6,t6,2
	add			t6,a2,t6
	lw			t6,0(t6)
	subw		t4,t4,t6
	add			t6,t4,x0
	jal			x0,.long_func_351
.long_func_340:
	addiw		t3,x0,0
	blt			t6,t3,.long_func_343
.long_func_344:
	addiw		t3,x0,0
.long_func_345:
	add			a3,t6,x0
	jal			x0,.long_func_342
.long_func_343:
	lui			t3,16
	addiw		t3,t3,-1
	jal			x0,.long_func_345
.long_func_313:
	addiw		t3,x0,0
	addiw		a0,x0,0
	add			a1,t4,x0
	add			a2,t5,x0
	add			a3,a0,x0
.long_func_315:
	addiw		a0,x0,16
	blt			a3,a0,.long_func_316
.long_func_317:
	addiw		a0,x0,0
	addiw		a1,x0,0
	add			a4,a1,x0
.long_func_325:
	addiw		a1,x0,16
	blt			a4,a1,.long_func_326
.long_func_327:
	addiw		a1,x0,1
	addiw		a2,x0,15
	bgt			a1,a2,.long_func_331
.long_func_332:
	addiw		t4,x0,0
	addiw		t5,x0,0
	addiw		a1,x0,1
	lui			a2,%hi(SHIFT_TABLE)
	addi		a2,a2,%lo(SHIFT_TABLE)
	slli		a1,a1,2
	add			a1,a2,a1
	lw			a1,0(a1)
	mulw		a0,a0,a1
	lui			a1,16
	addiw		a1,a1,-1
	add			a2,a1,x0
.long_func_334:
	addiw		a1,x0,16
	blt			t5,a1,.long_func_335
.long_func_336:
	add			a1,t4,x0
	add			a3,a0,x0
	add			t4,t5,x0
.long_func_333:
	add			t5,t3,x0
	add			a0,t4,x0
	add			t4,t3,x0
	add			t4,a1,x0
	add			a2,a3,x0
	add			t3,a1,x0
	jal			x0,.long_func_312
.long_func_335:
	addiw		a1,x0,2
	srliw		a1,a0,31
	add			a1,a1,a0
	andi		a1,a1,-2
	subw		a1,a0,a1
	bne			a1,x0,.long_func_339
.long_func_798:
	add			a1,t4,x0
.long_func_338:
	addiw		t4,x0,2
	srliw		t4,a0,31
	add			t4,a0,t4
	sraiw		a0,t4,1
	addiw		t4,x0,2
	srliw		t4,a2,31
	add			t4,a2,t4
	sraiw		t4,t4,1
	addiw		a2,x0,1
	addw		t5,t5,a2
	add			a2,t4,x0
	add			t4,a1,x0
	jal			x0,.long_func_334
.long_func_339:
	addiw		a1,x0,2
	srliw		a1,a2,31
	add			a1,a1,a2
	andi		a1,a1,-2
	subw		a1,a2,a1
	bne			a1,x0,.long_func_337
.long_func_799:
	add			a1,t4,x0
	jal			x0,.long_func_338
.long_func_337:
	addiw		a1,x0,1
	add			a1,t5,x0
	lui			a3,%hi(SHIFT_TABLE)
	addi		a3,a3,%lo(SHIFT_TABLE)
	slli		a1,a1,2
	add			a1,a3,a1
	lw			a1,0(a1)
	addw		t4,t4,a1
	add			a1,t4,x0
	jal			x0,.long_func_338
.long_func_331:
	addiw		a0,x0,0
	add			a1,a0,x0
	add			a2,t4,x0
	add			a3,t5,x0
	add			t4,a4,x0
	jal			x0,.long_func_333
.long_func_326:
	addiw		a1,x0,2
	srliw		a1,t5,31
	add			a1,a1,t5
	andi		a1,a1,-2
	subw		a1,t5,a1
	bne			a1,x0,.long_func_330
.long_func_796:
	add			a1,a0,x0
.long_func_329:
	addiw		a0,x0,2
	srliw		a0,t5,31
	add			t5,t5,a0
	sraiw		t5,t5,1
	addiw		a0,x0,2
	srliw		a0,t4,31
	add			t4,t4,a0
	sraiw		t4,t4,1
	addiw		a0,x0,1
	addw		a0,a4,a0
	add			a4,a0,x0
	add			a0,a1,x0
	jal			x0,.long_func_325
.long_func_330:
	addiw		a1,x0,2
	srliw		a1,t4,31
	add			a1,a1,t4
	andi		a1,a1,-2
	subw		a1,t4,a1
	bne			a1,x0,.long_func_328
.long_func_797:
	add			a1,a0,x0
	jal			x0,.long_func_329
.long_func_328:
	addiw		a1,x0,1
	add			a1,a4,x0
	lui			a2,%hi(SHIFT_TABLE)
	addi		a2,a2,%lo(SHIFT_TABLE)
	slli		a1,a1,2
	add			a1,a2,a1
	lw			a1,0(a1)
	addw		a0,a0,a1
	add			a1,a0,x0
	jal			x0,.long_func_329
.long_func_316:
	addiw		a0,x0,2
	srliw		a0,a2,31
	add			a0,a0,a2
	andi		a0,a0,-2
	subw		a0,a2,a0
	bne			a0,x0,.long_func_318
.long_func_319:
	addiw		a0,x0,2
	srliw		a0,a1,31
	add			a0,a0,a1
	andi		a0,a0,-2
	subw		a0,a1,a0
	bne			a0,x0,.long_func_323
.long_func_795:
.long_func_324:
.long_func_320:
	addiw		a0,x0,2
	srliw		a0,a2,31
	add			a0,a2,a0
	sraiw		a2,a0,1
	addiw		a0,x0,2
	srliw		a0,a1,31
	add			a0,a1,a0
	sraiw		a0,a0,1
	addiw		a1,x0,1
	addw		a3,a3,a1
	add			a1,a0,x0
	jal			x0,.long_func_315
.long_func_323:
	addiw		a0,x0,1
	add			a0,a3,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		a0,a0,2
	add			a0,a4,a0
	lw			a0,0(a0)
	addw		t3,t3,a0
	jal			x0,.long_func_324
.long_func_318:
	addiw		a0,x0,2
	srliw		a0,a1,31
	add			a0,a0,a1
	andi		a0,a0,-2
	subw		a0,a1,a0
	addiw		a4,x0,0
	beq			a0,a4,.long_func_321
.long_func_794:
.long_func_322:
	jal			x0,.long_func_320
.long_func_321:
	addiw		a0,x0,1
	add			a0,a3,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		a0,a0,2
	add			a0,a4,a0
	lw			a0,0(a0)
	addw		t3,t3,a0
	jal			x0,.long_func_322
.long_func_282:
	addiw		t2,x0,0
	add			a3,a2,x0
	add			t2,t3,x0
.long_func_284:
	bne			a3,x0,.long_func_285
.long_func_286:
	add			t2,t4,x0
	add			a3,t5,x0
	add			a4,a0,x0
	add			a0,a1,x0
	add			t3,t4,x0
	jal			x0,.long_func_283
.long_func_285:
	addiw		t2,x0,0
	addiw		t3,x0,0
	add			t5,a3,x0
	add			a0,t4,x0
	add			a1,t3,x0
.long_func_287:
	addiw		t3,x0,16
	blt			a1,t3,.long_func_288
.long_func_289:
	addiw		t3,x0,0
	addiw		t5,x0,0
	add			a0,a3,x0
	add			a1,t5,x0
.long_func_297:
	addiw		t5,x0,16
	blt			a1,t5,.long_func_298
.long_func_299:
	addiw		t5,x0,1
	addiw		a3,x0,15
	bgt			t5,a3,.long_func_303
.long_func_304:
	addiw		t4,x0,0
	addiw		t5,x0,0
	addiw		a0,x0,1
	lui			a1,%hi(SHIFT_TABLE)
	addi		a1,a1,%lo(SHIFT_TABLE)
	slli		a0,a0,2
	add			a0,a1,a0
	lw			a0,0(a0)
	mulw		t3,t3,a0
	lui			a0,16
	addiw		a0,a0,-1
	add			a1,t3,x0
	add			a3,t5,x0
	add			t3,t4,x0
.long_func_306:
	addiw		t4,x0,16
	blt			a3,t4,.long_func_307
.long_func_308:
	add			t5,a0,x0
	add			a0,a1,x0
	add			a1,a3,x0
.long_func_305:
	add			t4,t2,x0
	add			a3,t3,x0
	add			t2,t3,x0
	jal			x0,.long_func_284
.long_func_307:
	addiw		t4,x0,2
	srliw		t4,a1,31
	add			t4,t4,a1
	andi		t4,t4,-2
	subw		t4,a1,t4
	bne			t4,x0,.long_func_311
.long_func_792:
	add			a4,t3,x0
.long_func_310:
	addiw		t3,x0,2
	srliw		t3,a1,31
	add			t3,a1,t3
	sraiw		t4,t3,1
	addiw		t3,x0,2
	srliw		t3,a0,31
	add			t3,a0,t3
	sraiw		t3,t3,1
	addiw		t5,x0,1
	addw		t5,a3,t5
	add			a0,t3,x0
	add			a1,t4,x0
	add			a3,t5,x0
	add			t3,a4,x0
	jal			x0,.long_func_306
.long_func_311:
	addiw		t4,x0,2
	srliw		t4,a0,31
	add			t4,t4,a0
	andi		t4,t4,-2
	subw		t4,a0,t4
	bne			t4,x0,.long_func_309
.long_func_793:
	add			a4,t3,x0
	jal			x0,.long_func_310
.long_func_309:
	addiw		t4,x0,1
	add			t4,a3,x0
	lui			t5,%hi(SHIFT_TABLE)
	addi		t5,t5,%lo(SHIFT_TABLE)
	slli		t4,t4,2
	add			t4,t5,t4
	lw			t4,0(t4)
	addw		t3,t3,t4
	add			a4,t3,x0
	jal			x0,.long_func_310
.long_func_303:
	addiw		t3,x0,0
	add			t5,a0,x0
	add			a0,t4,x0
	jal			x0,.long_func_305
.long_func_298:
	addiw		t5,x0,2
	srliw		t5,t4,31
	add			t5,t5,t4
	andi		t5,t5,-2
	subw		t5,t4,t5
	bne			t5,x0,.long_func_302
.long_func_790:
	add			a3,t3,x0
.long_func_301:
	addiw		t3,x0,2
	srliw		t3,t4,31
	add			t3,t4,t3
	sraiw		t4,t3,1
	addiw		t3,x0,2
	srliw		t3,a0,31
	add			t3,a0,t3
	sraiw		t3,t3,1
	addiw		t5,x0,1
	addw		t5,a1,t5
	add			a0,t3,x0
	add			a1,t5,x0
	add			t3,a3,x0
	jal			x0,.long_func_297
.long_func_302:
	addiw		t5,x0,2
	srliw		t5,a0,31
	add			t5,t5,a0
	andi		t5,t5,-2
	subw		t5,a0,t5
	bne			t5,x0,.long_func_300
.long_func_791:
	add			a3,t3,x0
	jal			x0,.long_func_301
.long_func_300:
	addiw		t5,x0,1
	add			t5,a1,x0
	lui			a3,%hi(SHIFT_TABLE)
	addi		a3,a3,%lo(SHIFT_TABLE)
	slli		t5,t5,2
	add			t5,a3,t5
	lw			t5,0(t5)
	addw		t3,t3,t5
	add			a3,t3,x0
	jal			x0,.long_func_301
.long_func_288:
	addiw		t3,x0,2
	srliw		t3,a0,31
	add			t3,t3,a0
	andi		t3,t3,-2
	subw		t3,a0,t3
	bne			t3,x0,.long_func_290
.long_func_291:
	addiw		t3,x0,2
	srliw		t3,t5,31
	add			t3,t3,t5
	andi		t3,t3,-2
	subw		t3,t5,t3
	bne			t3,x0,.long_func_295
.long_func_789:
.long_func_296:
.long_func_292:
	addiw		t3,x0,2
	srliw		t3,a0,31
	add			t3,a0,t3
	sraiw		a0,t3,1
	addiw		t3,x0,2
	srliw		t3,t5,31
	add			t3,t5,t3
	sraiw		t3,t3,1
	addiw		t5,x0,1
	addw		a1,a1,t5
	add			t5,t3,x0
	jal			x0,.long_func_287
.long_func_295:
	addiw		t3,x0,1
	add			t3,a1,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t3,t3,2
	add			t3,a4,t3
	lw			t3,0(t3)
	addw		t2,t2,t3
	jal			x0,.long_func_296
.long_func_290:
	addiw		t3,x0,2
	srliw		t3,t5,31
	add			t3,t3,t5
	andi		t3,t3,-2
	subw		t3,t5,t3
	addiw		a4,x0,0
	beq			t3,a4,.long_func_293
.long_func_788:
.long_func_294:
	jal			x0,.long_func_292
.long_func_293:
	addiw		t3,x0,1
	add			t3,a1,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t3,t3,2
	add			t3,a4,t3
	lw			t3,0(t3)
	addw		t2,t2,t3
	jal			x0,.long_func_294
.long_func_277:
	addiw		t2,x0,2
	srliw		t2,a0,31
	add			t2,t2,a0
	andi		t2,t2,-2
	subw		t2,a0,t2
	bne			t2,x0,.long_func_281
.long_func_785:
	add			a3,t3,x0
.long_func_280:
	addiw		t2,x0,2
	srliw		t2,a0,31
	add			t2,a0,t2
	sraiw		t3,t2,1
	addiw		t2,x0,2
	srliw		t2,t5,31
	add			t2,t5,t2
	sraiw		t2,t2,1
	addiw		t5,x0,1
	addw		a1,a1,t5
	add			t5,t2,x0
	add			a0,t3,x0
	add			t3,a3,x0
	jal			x0,.long_func_276
.long_func_281:
	addiw		t2,x0,2
	srliw		t2,t5,31
	add			t2,t2,t5
	andi		t2,t2,-2
	subw		t2,t5,t2
	bne			t2,x0,.long_func_279
.long_func_786:
	add			a3,t3,x0
	jal			x0,.long_func_280
.long_func_279:
	addiw		t2,x0,1
	add			t2,a1,x0
	lui			a3,%hi(SHIFT_TABLE)
	addi		a3,a3,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,a3,t2
	lw			t2,0(t2)
	addw		t2,t3,t2
	add			a3,t2,x0
	jal			x0,.long_func_280
.long_func_192:
	addiw		t0,x0,0
	add			a0,t0,x0
	add			a1,t3,x0
	add			a2,s4,x0
	add			t0,t2,x0
.long_func_194:
	bne			a1,x0,.long_func_195
.long_func_196:
	add			t0,a0,x0
	add			a1,t4,x0
	add			t2,a0,x0
	jal			x0,.long_func_193
.long_func_195:
	addiw		t0,x0,0
	addiw		t2,x0,0
	addiw		t4,x0,1
	add			t5,a1,x0
	add			t6,t2,x0
	add			t2,t0,x0
.long_func_197:
	addiw		t0,x0,16
	blt			t6,t0,.long_func_198
.long_func_199:
	bne			t2,x0,.long_func_203
.long_func_772:
	add			t0,a0,x0
.long_func_204:
	addiw		a0,x0,0
	add			a3,a2,x0
	add			a0,a2,x0
	add			a2,t4,x0
.long_func_233:
	bne			a0,x0,.long_func_234
.long_func_235:
	addiw		t4,x0,1
	addiw		t2,x0,15
	bge			t4,t2,.long_func_261
.long_func_262:
	addiw		t2,x0,0
	bgt			t4,t2,.long_func_267
.long_func_268:
	add			t5,a1,x0
	add			t2,a1,x0
.long_func_269:
.long_func_263:
	add			a0,t0,x0
	add			a1,t2,x0
	add			a2,a3,x0
	add			t0,t2,x0
	jal			x0,.long_func_194
.long_func_267:
	lui			t2,8
	addiw		t2,t2,-1
	bgt			a1,t2,.long_func_270
.long_func_271:
	add			t2,t4,x0
	lui			t5,%hi(SHIFT_TABLE)
	addi		t5,t5,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,t5,t2
	lw			t2,0(t2)
	divw		t5,a1,t2
	add			t2,a1,x0
	add			a0,t5,x0
.long_func_272:
	add			t5,t2,x0
	add			t2,a0,x0
	jal			x0,.long_func_269
.long_func_270:
	add			t2,t4,x0
	lui			t5,%hi(SHIFT_TABLE)
	addi		t5,t5,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,t5,t2
	lw			t2,0(t2)
	divw		t2,a1,t2
	lui			t5,16
	addw		t5,t2,t5
	addiw		a0,x0,15
	subw		a0,a0,t4
	addiw		a1,x0,1
	addw		a0,a0,a1
	lui			a1,%hi(SHIFT_TABLE)
	addi		a1,a1,%lo(SHIFT_TABLE)
	slli		a0,a0,2
	add			a0,a1,a0
	lw			a0,0(a0)
	subw		t5,t5,a0
	add			a0,t5,x0
	jal			x0,.long_func_272
.long_func_261:
	addiw		t2,x0,0
	blt			a1,t2,.long_func_264
.long_func_265:
	addiw		t2,x0,0
.long_func_266:
	add			t5,a1,x0
	jal			x0,.long_func_263
.long_func_264:
	lui			t2,16
	addiw		t2,t2,-1
	jal			x0,.long_func_266
.long_func_234:
	addiw		t2,x0,0
	addiw		t4,x0,0
	add			t5,a0,x0
	add			t6,a3,x0
	add			a2,t4,x0
.long_func_236:
	addiw		t4,x0,16
	blt			a2,t4,.long_func_237
.long_func_238:
	addiw		t4,x0,0
	addiw		t5,x0,0
	add			t6,a0,x0
	add			a0,a3,x0
	add			a2,t5,x0
.long_func_246:
	addiw		t5,x0,16
	blt			a2,t5,.long_func_247
.long_func_248:
	addiw		t5,x0,1
	addiw		a3,x0,15
	bgt			t5,a3,.long_func_252
.long_func_253:
	addiw		t5,x0,0
	addiw		t6,x0,0
	addiw		a0,x0,1
	lui			a2,%hi(SHIFT_TABLE)
	addi		a2,a2,%lo(SHIFT_TABLE)
	slli		a0,a0,2
	add			a0,a2,a0
	lw			a0,0(a0)
	mulw		t4,t4,a0
	lui			a0,16
	addiw		a0,a0,-1
	add			a2,t4,x0
	add			t4,t5,x0
.long_func_255:
	addiw		t5,x0,16
	blt			t6,t5,.long_func_256
.long_func_257:
	add			t5,a0,x0
	add			a4,a2,x0
.long_func_254:
	add			a3,t2,x0
	add			a0,t2,x0
	add			a0,t4,x0
	add			a2,t5,x0
	add			t5,a4,x0
	add			t2,t4,x0
	jal			x0,.long_func_233
.long_func_256:
	addiw		t5,x0,2
	srliw		t5,a2,31
	add			t5,t5,a2
	andi		t5,t5,-2
	subw		t5,a2,t5
	bne			t5,x0,.long_func_260
.long_func_783:
	add			a3,t4,x0
.long_func_259:
	addiw		t4,x0,2
	srliw		t4,a2,31
	add			t4,a2,t4
	sraiw		t5,t4,1
	addiw		t4,x0,2
	srliw		t4,a0,31
	add			t4,a0,t4
	sraiw		t4,t4,1
	addiw		a0,x0,1
	addw		t6,t6,a0
	add			a0,t4,x0
	add			a2,t5,x0
	add			t4,a3,x0
	jal			x0,.long_func_255
.long_func_260:
	addiw		t5,x0,2
	srliw		t5,a0,31
	add			t5,t5,a0
	andi		t5,t5,-2
	subw		t5,a0,t5
	bne			t5,x0,.long_func_258
.long_func_784:
	add			a3,t4,x0
	jal			x0,.long_func_259
.long_func_258:
	addiw		t5,x0,1
	add			t5,t6,x0
	lui			a3,%hi(SHIFT_TABLE)
	addi		a3,a3,%lo(SHIFT_TABLE)
	slli		t5,t5,2
	add			t5,a3,t5
	lw			t5,0(t5)
	addw		t4,t4,t5
	add			a3,t4,x0
	jal			x0,.long_func_259
.long_func_252:
	addiw		t4,x0,0
	add			t5,t6,x0
	add			a4,a0,x0
	add			t6,a2,x0
	jal			x0,.long_func_254
.long_func_247:
	addiw		t5,x0,2
	srliw		t5,a0,31
	add			t5,t5,a0
	andi		t5,t5,-2
	subw		t5,a0,t5
	bne			t5,x0,.long_func_251
.long_func_781:
	add			a3,t4,x0
.long_func_250:
	addiw		t4,x0,2
	srliw		t4,a0,31
	add			t4,a0,t4
	sraiw		t5,t4,1
	addiw		t4,x0,2
	srliw		t4,t6,31
	add			t4,t6,t4
	sraiw		t4,t4,1
	addiw		t6,x0,1
	addw		a2,a2,t6
	add			t6,t4,x0
	add			a0,t5,x0
	add			t4,a3,x0
	jal			x0,.long_func_246
.long_func_251:
	addiw		t5,x0,2
	srliw		t5,t6,31
	add			t5,t5,t6
	andi		t5,t5,-2
	subw		t5,t6,t5
	bne			t5,x0,.long_func_249
.long_func_782:
	add			a3,t4,x0
	jal			x0,.long_func_250
.long_func_249:
	addiw		t5,x0,1
	add			t5,a2,x0
	lui			a3,%hi(SHIFT_TABLE)
	addi		a3,a3,%lo(SHIFT_TABLE)
	slli		t5,t5,2
	add			t5,a3,t5
	lw			t5,0(t5)
	addw		t4,t4,t5
	add			a3,t4,x0
	jal			x0,.long_func_250
.long_func_237:
	addiw		t4,x0,2
	srliw		t4,t6,31
	add			t4,t4,t6
	andi		t4,t4,-2
	subw		t4,t6,t4
	bne			t4,x0,.long_func_239
.long_func_240:
	addiw		t4,x0,2
	srliw		t4,t5,31
	add			t4,t4,t5
	andi		t4,t4,-2
	subw		t4,t5,t4
	bne			t4,x0,.long_func_244
.long_func_780:
.long_func_245:
.long_func_241:
	addiw		t4,x0,2
	srliw		t4,t6,31
	add			t4,t6,t4
	sraiw		t6,t4,1
	addiw		t4,x0,2
	srliw		t4,t5,31
	add			t4,t5,t4
	sraiw		t4,t4,1
	addiw		t5,x0,1
	addw		a2,a2,t5
	add			t5,t4,x0
	jal			x0,.long_func_236
.long_func_244:
	addiw		t4,x0,1
	add			t4,a2,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t4,t4,2
	add			t4,a4,t4
	lw			t4,0(t4)
	addw		t2,t2,t4
	jal			x0,.long_func_245
.long_func_239:
	addiw		t4,x0,2
	srliw		t4,t5,31
	add			t4,t4,t5
	andi		t4,t4,-2
	subw		t4,t5,t4
	addiw		a4,x0,0
	beq			t4,a4,.long_func_242
.long_func_779:
.long_func_243:
	jal			x0,.long_func_241
.long_func_242:
	addiw		t4,x0,1
	add			t4,a2,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t4,t4,2
	add			t4,a4,t4
	lw			t4,0(t4)
	addw		t2,t2,t4
	jal			x0,.long_func_243
.long_func_203:
	addiw		t0,x0,0
	add			a3,a2,x0
	add			t0,t2,x0
.long_func_205:
	bne			a3,x0,.long_func_206
.long_func_207:
	add			t0,a0,x0
	add			t2,a0,x0
	jal			x0,.long_func_204
.long_func_206:
	addiw		t0,x0,0
	addiw		t2,x0,0
	add			t4,a3,x0
	add			t5,a0,x0
	add			t6,t2,x0
.long_func_208:
	addiw		t2,x0,16
	blt			t6,t2,.long_func_209
.long_func_210:
	addiw		t2,x0,0
	addiw		t4,x0,0
	add			t5,a3,x0
	add			t6,a0,x0
	add			a0,t4,x0
.long_func_218:
	addiw		t4,x0,16
	blt			a0,t4,.long_func_219
.long_func_220:
	addiw		t4,x0,1
	addiw		a3,x0,15
	bgt			t4,a3,.long_func_224
.long_func_225:
	addiw		t4,x0,0
	addiw		t5,x0,0
	addiw		t6,x0,1
	lui			a0,%hi(SHIFT_TABLE)
	addi		a0,a0,%lo(SHIFT_TABLE)
	slli		t6,t6,2
	add			t6,a0,t6
	lw			t6,0(t6)
	mulw		t2,t2,t6
	lui			t6,16
	addiw		t6,t6,-1
	add			a0,t2,x0
	add			a3,t5,x0
	add			t2,t4,x0
.long_func_227:
	addiw		t4,x0,16
	blt			a3,t4,.long_func_228
.long_func_229:
	add			t4,t6,x0
	add			t5,a0,x0
	add			t6,a3,x0
.long_func_226:
	add			a0,t0,x0
	add			a3,t2,x0
	add			t0,t2,x0
	jal			x0,.long_func_205
.long_func_228:
	addiw		t4,x0,2
	srliw		t4,a0,31
	add			t4,t4,a0
	andi		t4,t4,-2
	subw		t4,a0,t4
	bne			t4,x0,.long_func_232
.long_func_777:
	add			a4,t2,x0
.long_func_231:
	addiw		t2,x0,2
	srliw		t2,a0,31
	add			t2,a0,t2
	sraiw		t4,t2,1
	addiw		t2,x0,2
	srliw		t2,t6,31
	add			t2,t6,t2
	sraiw		t2,t2,1
	addiw		t5,x0,1
	addw		t5,a3,t5
	add			t6,t2,x0
	add			a0,t4,x0
	add			a3,t5,x0
	add			t2,a4,x0
	jal			x0,.long_func_227
.long_func_232:
	addiw		t4,x0,2
	srliw		t4,t6,31
	add			t4,t4,t6
	andi		t4,t4,-2
	subw		t4,t6,t4
	bne			t4,x0,.long_func_230
.long_func_778:
	add			a4,t2,x0
	jal			x0,.long_func_231
.long_func_230:
	addiw		t4,x0,1
	add			t4,a3,x0
	lui			t5,%hi(SHIFT_TABLE)
	addi		t5,t5,%lo(SHIFT_TABLE)
	slli		t4,t4,2
	add			t4,t5,t4
	lw			t4,0(t4)
	addw		t2,t2,t4
	add			a4,t2,x0
	jal			x0,.long_func_231
.long_func_224:
	addiw		t2,x0,0
	add			t4,t5,x0
	add			t5,t6,x0
	add			t6,a0,x0
	jal			x0,.long_func_226
.long_func_219:
	addiw		t4,x0,2
	srliw		t4,t6,31
	add			t4,t4,t6
	andi		t4,t4,-2
	subw		t4,t6,t4
	bne			t4,x0,.long_func_223
.long_func_775:
	add			a3,t2,x0
.long_func_222:
	addiw		t2,x0,2
	srliw		t2,t6,31
	add			t2,t6,t2
	sraiw		t4,t2,1
	addiw		t2,x0,2
	srliw		t2,t5,31
	add			t2,t5,t2
	sraiw		t2,t2,1
	addiw		t5,x0,1
	addw		a0,a0,t5
	add			t5,t2,x0
	add			t6,t4,x0
	add			t2,a3,x0
	jal			x0,.long_func_218
.long_func_223:
	addiw		t4,x0,2
	srliw		t4,t5,31
	add			t4,t4,t5
	andi		t4,t4,-2
	subw		t4,t5,t4
	bne			t4,x0,.long_func_221
.long_func_776:
	add			a3,t2,x0
	jal			x0,.long_func_222
.long_func_221:
	addiw		t4,x0,1
	add			t4,a0,x0
	lui			a3,%hi(SHIFT_TABLE)
	addi		a3,a3,%lo(SHIFT_TABLE)
	slli		t4,t4,2
	add			t4,a3,t4
	lw			t4,0(t4)
	addw		t2,t2,t4
	add			a3,t2,x0
	jal			x0,.long_func_222
.long_func_209:
	addiw		t2,x0,2
	srliw		t2,t5,31
	add			t2,t2,t5
	andi		t2,t2,-2
	subw		t2,t5,t2
	bne			t2,x0,.long_func_211
.long_func_212:
	addiw		t2,x0,2
	srliw		t2,t4,31
	add			t2,t2,t4
	andi		t2,t2,-2
	subw		t2,t4,t2
	bne			t2,x0,.long_func_216
.long_func_774:
.long_func_217:
.long_func_213:
	addiw		t2,x0,2
	srliw		t2,t5,31
	add			t2,t5,t2
	sraiw		t5,t2,1
	addiw		t2,x0,2
	srliw		t2,t4,31
	add			t2,t4,t2
	sraiw		t2,t2,1
	addiw		t4,x0,1
	addw		t6,t6,t4
	add			t4,t2,x0
	jal			x0,.long_func_208
.long_func_216:
	addiw		t2,x0,1
	add			t2,t6,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,a4,t2
	lw			t2,0(t2)
	addw		t0,t0,t2
	jal			x0,.long_func_217
.long_func_211:
	addiw		t2,x0,2
	srliw		t2,t4,31
	add			t2,t2,t4
	andi		t2,t2,-2
	subw		t2,t4,t2
	addiw		a4,x0,0
	beq			t2,a4,.long_func_214
.long_func_773:
.long_func_215:
	jal			x0,.long_func_213
.long_func_214:
	addiw		t2,x0,1
	add			t2,t6,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,a4,t2
	lw			t2,0(t2)
	addw		t0,t0,t2
	jal			x0,.long_func_215
.long_func_198:
	addiw		t0,x0,2
	srliw		t0,t5,31
	add			t0,t0,t5
	andi		t0,t0,-2
	subw		t0,t5,t0
	bne			t0,x0,.long_func_202
.long_func_770:
	add			a3,t2,x0
.long_func_201:
	addiw		t0,x0,2
	srliw		t0,t5,31
	add			t0,t5,t0
	sraiw		t2,t0,1
	addiw		t0,x0,2
	srliw		t0,t4,31
	add			t0,t4,t0
	sraiw		t0,t0,1
	addiw		t4,x0,1
	addw		t6,t6,t4
	add			t4,t0,x0
	add			t5,t2,x0
	add			t2,a3,x0
	jal			x0,.long_func_197
.long_func_202:
	addiw		t0,x0,2
	srliw		t0,t4,31
	add			t0,t0,t4
	andi		t0,t0,-2
	subw		t0,t4,t0
	bne			t0,x0,.long_func_200
.long_func_771:
	add			a3,t2,x0
	jal			x0,.long_func_201
.long_func_200:
	addiw		t0,x0,1
	add			t0,t6,x0
	lui			a3,%hi(SHIFT_TABLE)
	addi		a3,a3,%lo(SHIFT_TABLE)
	slli		t0,t0,2
	add			t0,a3,t0
	lw			t0,0(t0)
	addw		t0,t2,t0
	add			a3,t0,x0
	jal			x0,.long_func_201
.long_func_187:
	addiw		t0,x0,2
	srliw		t0,t5,31
	add			t0,t0,t5
	andi		t0,t0,-2
	subw		t0,t5,t0
	bne			t0,x0,.long_func_191
.long_func_767:
	add			a0,t2,x0
.long_func_190:
	addiw		t0,x0,2
	srliw		t0,t5,31
	add			t0,t5,t0
	sraiw		t2,t0,1
	addiw		t0,x0,2
	srliw		t0,t4,31
	add			t0,t4,t0
	sraiw		t0,t0,1
	addiw		t4,x0,1
	addw		t6,t6,t4
	add			t4,t0,x0
	add			t5,t2,x0
	add			t2,a0,x0
	jal			x0,.long_func_186
.long_func_191:
	addiw		t0,x0,2
	srliw		t0,t4,31
	add			t0,t0,t4
	andi		t0,t0,-2
	subw		t0,t4,t0
	bne			t0,x0,.long_func_189
.long_func_768:
	add			a0,t2,x0
	jal			x0,.long_func_190
.long_func_189:
	addiw		t0,x0,1
	add			t0,t6,x0
	lui			a0,%hi(SHIFT_TABLE)
	addi		a0,a0,%lo(SHIFT_TABLE)
	slli		t0,t0,2
	add			t0,a0,t0
	lw			t0,0(t0)
	addw		t0,t2,t0
	add			a0,t0,x0
	jal			x0,.long_func_190
.long_func_3:
	addiw		t0,x0,0
	addiw		t1,x0,0
	addiw		t2,x0,1
	add			t3,t5,x0
	add			t6,t1,x0
	add			t1,t0,x0
.long_func_5:
	addiw		t0,x0,16
	blt			t6,t0,.long_func_6
.long_func_7:
	bne			t1,x0,.long_func_11
.long_func_736:
	add			t0,fp,x0
	add			a1,t2,x0
.long_func_12:
	addiw		t2,x0,0
	add			a0,t6,x0
	add			t6,t4,x0
	add			a1,t3,x0
.long_func_92:
	bne			t6,x0,.long_func_93
.long_func_94:
	addiw		t3,x0,1
	addiw		t1,x0,15
	bge			t3,t1,.long_func_171
.long_func_172:
	addiw		t1,x0,0
	bgt			t3,t1,.long_func_177
.long_func_178:
	add			t4,t5,x0
	add			t1,t5,x0
.long_func_179:
	add			t6,t4,x0
.long_func_173:
	add			fp,t0,x0
	add			t5,t1,x0
	add			t4,t2,x0
	add			s1,t3,x0
	add			s2,t6,x0
	add			s3,a0,x0
	add			t0,t1,x0
	jal			x0,.long_func_2
.long_func_177:
	lui			t1,8
	addiw		t1,t1,-1
	bgt			t5,t1,.long_func_180
.long_func_181:
	add			t1,t3,x0
	lui			t4,%hi(SHIFT_TABLE)
	addi		t4,t4,%lo(SHIFT_TABLE)
	slli		t1,t1,2
	add			t1,t4,t1
	lw			t1,0(t1)
	divw		t4,t5,t1
	add			t1,t5,x0
	add			t5,t4,x0
.long_func_182:
	add			t4,t1,x0
	add			t1,t5,x0
	jal			x0,.long_func_179
.long_func_180:
	add			t1,t3,x0
	lui			t4,%hi(SHIFT_TABLE)
	addi		t4,t4,%lo(SHIFT_TABLE)
	slli		t1,t1,2
	add			t1,t4,t1
	lw			t1,0(t1)
	divw		t1,t5,t1
	lui			t4,16
	addw		t4,t1,t4
	addiw		t5,x0,15
	subw		t5,t5,t3
	addiw		t6,x0,1
	addw		t5,t5,t6
	lui			t6,%hi(SHIFT_TABLE)
	addi		t6,t6,%lo(SHIFT_TABLE)
	slli		t5,t5,2
	add			t5,t6,t5
	lw			t5,0(t5)
	subw		t4,t4,t5
	add			t5,t4,x0
	jal			x0,.long_func_182
.long_func_171:
	addiw		t1,x0,0
	blt			t5,t1,.long_func_174
.long_func_175:
	addiw		t1,x0,0
.long_func_176:
	add			t6,t5,x0
	jal			x0,.long_func_173
.long_func_174:
	lui			t1,16
	addiw		t1,t1,-1
	jal			x0,.long_func_176
.long_func_93:
	addiw		t1,x0,0
	addiw		t3,x0,0
	addiw		a0,x0,1
	add			a1,t6,x0
	add			a2,t3,x0
	add			t3,t1,x0
.long_func_95:
	addiw		t1,x0,16
	blt			a2,t1,.long_func_96
.long_func_97:
	bne			t3,x0,.long_func_101
.long_func_754:
	add			t1,t2,x0
	add			a3,a0,x0
	add			a4,a1,x0
	add			a0,a2,x0
	add			t2,t3,x0
.long_func_102:
	addiw		t3,x0,0
	add			a1,t4,x0
	add			t3,t4,x0
	add			a2,a3,x0
	add			a2,a4,x0
.long_func_131:
	bne			t3,x0,.long_func_132
.long_func_133:
	addiw		a2,x0,1
	addiw		t2,x0,15
	bge			a2,t2,.long_func_159
.long_func_160:
	addiw		t2,x0,0
	bgt			a2,t2,.long_func_165
.long_func_166:
	add			t2,t6,x0
	add			t3,t6,x0
.long_func_167:
	add			a3,t2,x0
.long_func_161:
	add			t2,t1,x0
	add			t6,t3,x0
	add			t4,a1,x0
	add			a1,a2,x0
	add			a1,a3,x0
	add			t1,t3,x0
	jal			x0,.long_func_92
.long_func_165:
	lui			t2,8
	addiw		t2,t2,-1
	bgt			t6,t2,.long_func_168
.long_func_169:
	add			t2,a2,x0
	lui			t3,%hi(SHIFT_TABLE)
	addi		t3,t3,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,t3,t2
	lw			t2,0(t2)
	divw		t3,t6,t2
	add			t2,t6,x0
.long_func_170:
	jal			x0,.long_func_167
.long_func_168:
	add			t2,a2,x0
	lui			t3,%hi(SHIFT_TABLE)
	addi		t3,t3,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,t3,t2
	lw			t2,0(t2)
	divw		t2,t6,t2
	lui			t3,16
	addw		t3,t2,t3
	addiw		t4,x0,15
	subw		t4,t4,a2
	addiw		t6,x0,1
	addw		t4,t4,t6
	lui			t6,%hi(SHIFT_TABLE)
	addi		t6,t6,%lo(SHIFT_TABLE)
	slli		t4,t4,2
	add			t4,t6,t4
	lw			t4,0(t4)
	subw		t3,t3,t4
	jal			x0,.long_func_170
.long_func_159:
	addiw		t2,x0,0
	blt			t6,t2,.long_func_162
.long_func_163:
	addiw		t2,x0,0
.long_func_164:
	add			t3,t2,x0
	add			a3,t6,x0
	jal			x0,.long_func_161
.long_func_162:
	lui			t2,16
	addiw		t2,t2,-1
	jal			x0,.long_func_164
.long_func_132:
	addiw		t2,x0,0
	addiw		t4,x0,0
	add			a0,t3,x0
	add			a2,a1,x0
	add			a3,t4,x0
.long_func_134:
	addiw		t4,x0,16
	blt			a3,t4,.long_func_135
.long_func_136:
	addiw		t4,x0,0
	addiw		a0,x0,0
.long_func_144:
	addiw		a2,x0,16
	blt			a0,a2,.long_func_145
.long_func_146:
	addiw		a2,x0,1
	addiw		a3,x0,15
	bgt			a2,a3,.long_func_150
.long_func_151:
	addiw		t3,x0,0
	addiw		a0,x0,0
	addiw		a1,x0,1
	lui			a2,%hi(SHIFT_TABLE)
	addi		a2,a2,%lo(SHIFT_TABLE)
	slli		a1,a1,2
	add			a1,a2,a1
	lw			a1,0(a1)
	mulw		t4,t4,a1
	lui			a1,16
	addiw		a1,a1,-1
	add			a3,t4,x0
.long_func_153:
	addiw		t4,x0,16
	blt			a0,t4,.long_func_154
.long_func_155:
	add			t4,t3,x0
	add			a2,a1,x0
	add			t3,a0,x0
.long_func_152:
	add			a1,t2,x0
	add			a0,t3,x0
	add			t3,t2,x0
	add			t3,t4,x0
	add			a2,a3,x0
	add			t2,t4,x0
	jal			x0,.long_func_131
.long_func_154:
	addiw		t4,x0,2
	srliw		t4,a3,31
	add			t4,t4,a3
	andi		t4,t4,-2
	subw		t4,a3,t4
	bne			t4,x0,.long_func_158
.long_func_765:
	add			a2,t3,x0
.long_func_157:
	addiw		t3,x0,2
	srliw		t3,a3,31
	add			t3,a3,t3
	sraiw		t4,t3,1
	addiw		t3,x0,2
	srliw		t3,a1,31
	add			t3,a1,t3
	sraiw		t3,t3,1
	addiw		a1,x0,1
	addw		a0,a0,a1
	add			a1,t3,x0
	add			a3,t4,x0
	add			t3,a2,x0
	jal			x0,.long_func_153
.long_func_158:
	addiw		t4,x0,2
	srliw		t4,a1,31
	add			t4,t4,a1
	andi		t4,t4,-2
	subw		t4,a1,t4
	bne			t4,x0,.long_func_156
.long_func_766:
	add			a2,t3,x0
	jal			x0,.long_func_157
.long_func_156:
	addiw		t4,x0,1
	add			t4,a0,x0
	lui			a2,%hi(SHIFT_TABLE)
	addi		a2,a2,%lo(SHIFT_TABLE)
	slli		t4,t4,2
	add			t4,a2,t4
	lw			t4,0(t4)
	addw		t3,t3,t4
	add			a2,t3,x0
	jal			x0,.long_func_157
.long_func_150:
	addiw		t4,x0,0
	add			a2,t3,x0
	add			a3,a1,x0
	add			t3,a0,x0
	jal			x0,.long_func_152
.long_func_145:
	addiw		a2,x0,2
	srliw		a2,a1,31
	add			a2,a2,a1
	andi		a2,a2,-2
	subw		a2,a1,a2
	bne			a2,x0,.long_func_149
.long_func_763:
	add			a2,t4,x0
.long_func_148:
	addiw		t4,x0,2
	srliw		t4,a1,31
	add			t4,a1,t4
	sraiw		t4,t4,1
	addiw		a1,x0,2
	srliw		a1,t3,31
	add			t3,t3,a1
	sraiw		t3,t3,1
	addiw		a1,x0,1
	addw		a0,a0,a1
	add			a1,t4,x0
	add			t4,a2,x0
	jal			x0,.long_func_144
.long_func_149:
	addiw		a2,x0,2
	srliw		a2,t3,31
	add			a2,a2,t3
	andi		a2,a2,-2
	subw		a2,t3,a2
	bne			a2,x0,.long_func_147
.long_func_764:
	add			a2,t4,x0
	jal			x0,.long_func_148
.long_func_147:
	addiw		a2,x0,1
	add			a2,a0,x0
	lui			a3,%hi(SHIFT_TABLE)
	addi		a3,a3,%lo(SHIFT_TABLE)
	slli		a2,a2,2
	add			a2,a3,a2
	lw			a2,0(a2)
	addw		t4,t4,a2
	add			a2,t4,x0
	jal			x0,.long_func_148
.long_func_135:
	addiw		t4,x0,2
	srliw		t4,a2,31
	add			t4,t4,a2
	andi		t4,t4,-2
	subw		t4,a2,t4
	bne			t4,x0,.long_func_137
.long_func_138:
	addiw		t4,x0,2
	srliw		t4,a0,31
	add			t4,t4,a0
	andi		t4,t4,-2
	subw		t4,a0,t4
	bne			t4,x0,.long_func_142
.long_func_762:
.long_func_143:
.long_func_139:
	addiw		t4,x0,2
	srliw		t4,a2,31
	add			t4,a2,t4
	sraiw		a2,t4,1
	addiw		t4,x0,2
	srliw		t4,a0,31
	add			t4,a0,t4
	sraiw		t4,t4,1
	addiw		a0,x0,1
	addw		a3,a3,a0
	add			a0,t4,x0
	jal			x0,.long_func_134
.long_func_142:
	addiw		t4,x0,1
	add			t4,a3,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t4,t4,2
	add			t4,a4,t4
	lw			t4,0(t4)
	addw		t2,t2,t4
	jal			x0,.long_func_143
.long_func_137:
	addiw		t4,x0,2
	srliw		t4,a0,31
	add			t4,t4,a0
	andi		t4,t4,-2
	subw		t4,a0,t4
	addiw		a4,x0,0
	beq			t4,a4,.long_func_140
.long_func_761:
.long_func_141:
	jal			x0,.long_func_139
.long_func_140:
	addiw		t4,x0,1
	add			t4,a3,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t4,t4,2
	add			t4,a4,t4
	lw			t4,0(t4)
	addw		t2,t2,t4
	jal			x0,.long_func_141
.long_func_101:
	addiw		t1,x0,0
	add			a3,t4,x0
	add			t1,t3,x0
.long_func_103:
	bne			a3,x0,.long_func_104
.long_func_105:
	add			t1,t2,x0
	add			a3,a0,x0
	add			a4,a1,x0
	add			a0,a2,x0
	jal			x0,.long_func_102
.long_func_104:
	addiw		t1,x0,0
	addiw		t3,x0,0
	add			a0,a3,x0
	add			a1,t2,x0
	add			a2,t3,x0
.long_func_106:
	addiw		t3,x0,16
	blt			a2,t3,.long_func_107
.long_func_108:
	addiw		t3,x0,0
	addiw		a0,x0,0
	add			a1,a3,x0
	add			a2,a0,x0
.long_func_116:
	addiw		a0,x0,16
	blt			a2,a0,.long_func_117
.long_func_118:
	addiw		a0,x0,1
	addiw		a3,x0,15
	bgt			a0,a3,.long_func_122
.long_func_123:
	addiw		t2,x0,0
	addiw		a0,x0,0
	addiw		a1,x0,1
	lui			a2,%hi(SHIFT_TABLE)
	addi		a2,a2,%lo(SHIFT_TABLE)
	slli		a1,a1,2
	add			a1,a2,a1
	lw			a1,0(a1)
	mulw		t3,t3,a1
	lui			a1,16
	addiw		a1,a1,-1
	add			a2,t3,x0
	add			a3,a0,x0
.long_func_125:
	addiw		t3,x0,16
	blt			a3,t3,.long_func_126
.long_func_127:
	add			t3,t2,x0
	add			a0,a1,x0
	add			a1,a2,x0
	add			a2,a3,x0
.long_func_124:
	add			t2,t1,x0
	add			a3,t3,x0
	add			t1,t3,x0
	jal			x0,.long_func_103
.long_func_126:
	addiw		t3,x0,2
	srliw		t3,a2,31
	add			t3,t3,a2
	andi		t3,t3,-2
	subw		t3,a2,t3
	bne			t3,x0,.long_func_130
.long_func_759:
	add			a4,t2,x0
.long_func_129:
	addiw		t2,x0,2
	srliw		t2,a2,31
	add			t2,a2,t2
	sraiw		t3,t2,1
	addiw		t2,x0,2
	srliw		t2,a1,31
	add			t2,a1,t2
	sraiw		t2,t2,1
	addiw		a0,x0,1
	addw		a0,a3,a0
	add			a1,t2,x0
	add			a2,t3,x0
	add			a3,a0,x0
	add			t2,a4,x0
	jal			x0,.long_func_125
.long_func_130:
	addiw		t3,x0,2
	srliw		t3,a1,31
	add			t3,t3,a1
	andi		t3,t3,-2
	subw		t3,a1,t3
	bne			t3,x0,.long_func_128
.long_func_760:
	add			a4,t2,x0
	jal			x0,.long_func_129
.long_func_128:
	addiw		t3,x0,1
	add			t3,a3,x0
	lui			a0,%hi(SHIFT_TABLE)
	addi		a0,a0,%lo(SHIFT_TABLE)
	slli		t3,t3,2
	add			t3,a0,t3
	lw			t3,0(t3)
	addw		t2,t2,t3
	add			a4,t2,x0
	jal			x0,.long_func_129
.long_func_122:
	addiw		t3,x0,0
	add			a0,a1,x0
	add			a1,t2,x0
	jal			x0,.long_func_124
.long_func_117:
	addiw		a0,x0,2
	srliw		a0,t2,31
	add			a0,a0,t2
	andi		a0,a0,-2
	subw		a0,t2,a0
	bne			a0,x0,.long_func_121
.long_func_757:
	add			a3,t3,x0
.long_func_120:
	addiw		t3,x0,2
	srliw		t3,t2,31
	add			t2,t2,t3
	sraiw		t3,t2,1
	addiw		t2,x0,2
	srliw		t2,a1,31
	add			t2,a1,t2
	sraiw		t2,t2,1
	addiw		a0,x0,1
	addw		a0,a2,a0
	add			a1,t2,x0
	add			t2,t3,x0
	add			a2,a0,x0
	add			t3,a3,x0
	jal			x0,.long_func_116
.long_func_121:
	addiw		a0,x0,2
	srliw		a0,a1,31
	add			a0,a0,a1
	andi		a0,a0,-2
	subw		a0,a1,a0
	bne			a0,x0,.long_func_119
.long_func_758:
	add			a3,t3,x0
	jal			x0,.long_func_120
.long_func_119:
	addiw		a0,x0,1
	add			a0,a2,x0
	lui			a3,%hi(SHIFT_TABLE)
	addi		a3,a3,%lo(SHIFT_TABLE)
	slli		a0,a0,2
	add			a0,a3,a0
	lw			a0,0(a0)
	addw		t3,t3,a0
	add			a3,t3,x0
	jal			x0,.long_func_120
.long_func_107:
	addiw		t3,x0,2
	srliw		t3,a1,31
	add			t3,t3,a1
	andi		t3,t3,-2
	subw		t3,a1,t3
	bne			t3,x0,.long_func_109
.long_func_110:
	addiw		t3,x0,2
	srliw		t3,a0,31
	add			t3,t3,a0
	andi		t3,t3,-2
	subw		t3,a0,t3
	bne			t3,x0,.long_func_114
.long_func_756:
.long_func_115:
.long_func_111:
	addiw		t3,x0,2
	srliw		t3,a1,31
	add			t3,a1,t3
	sraiw		a1,t3,1
	addiw		t3,x0,2
	srliw		t3,a0,31
	add			t3,a0,t3
	sraiw		t3,t3,1
	addiw		a0,x0,1
	addw		a2,a2,a0
	add			a0,t3,x0
	jal			x0,.long_func_106
.long_func_114:
	addiw		t3,x0,1
	add			t3,a2,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t3,t3,2
	add			t3,a4,t3
	lw			t3,0(t3)
	addw		t1,t1,t3
	jal			x0,.long_func_115
.long_func_109:
	addiw		t3,x0,2
	srliw		t3,a0,31
	add			t3,t3,a0
	andi		t3,t3,-2
	subw		t3,a0,t3
	addiw		a4,x0,0
	beq			t3,a4,.long_func_112
.long_func_755:
.long_func_113:
	jal			x0,.long_func_111
.long_func_112:
	addiw		t3,x0,1
	add			t3,a2,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t3,t3,2
	add			t3,a4,t3
	lw			t3,0(t3)
	addw		t1,t1,t3
	jal			x0,.long_func_113
.long_func_96:
	addiw		t1,x0,2
	srliw		t1,a1,31
	add			t1,t1,a1
	andi		t1,t1,-2
	subw		t1,a1,t1
	bne			t1,x0,.long_func_100
.long_func_752:
	add			a3,t3,x0
.long_func_99:
	addiw		t1,x0,2
	srliw		t1,a1,31
	add			t1,a1,t1
	sraiw		t3,t1,1
	addiw		t1,x0,2
	srliw		t1,a0,31
	add			t1,a0,t1
	sraiw		t1,t1,1
	addiw		a0,x0,1
	addw		a2,a2,a0
	add			a0,t1,x0
	add			a1,t3,x0
	add			t3,a3,x0
	jal			x0,.long_func_95
.long_func_100:
	addiw		t1,x0,2
	srliw		t1,a0,31
	add			t1,t1,a0
	andi		t1,t1,-2
	subw		t1,a0,t1
	bne			t1,x0,.long_func_98
.long_func_753:
	add			a3,t3,x0
	jal			x0,.long_func_99
.long_func_98:
	addiw		t1,x0,1
	add			t1,a2,x0
	lui			a3,%hi(SHIFT_TABLE)
	addi		a3,a3,%lo(SHIFT_TABLE)
	slli		t1,t1,2
	add			t1,a3,t1
	lw			t1,0(t1)
	addw		t1,t3,t1
	add			a3,t1,x0
	jal			x0,.long_func_99
.long_func_11:
	addiw		t0,x0,0
	add			a0,t0,x0
	add			a1,t4,x0
	add			a2,fp,x0
	add			t0,t1,x0
.long_func_13:
	bne			a1,x0,.long_func_14
.long_func_15:
	add			t0,a0,x0
	add			a1,t2,x0
	add			t1,a0,x0
	jal			x0,.long_func_12
.long_func_14:
	addiw		t0,x0,0
	addiw		t1,x0,0
	addiw		t2,x0,1
	add			t3,a1,x0
	add			t6,t1,x0
	add			t1,t0,x0
.long_func_16:
	addiw		t0,x0,16
	blt			t6,t0,.long_func_17
.long_func_18:
	bne			t1,x0,.long_func_22
.long_func_739:
	add			t0,a0,x0
.long_func_23:
	addiw		a0,x0,0
	add			a3,a2,x0
	add			a0,a2,x0
	add			a2,t2,x0
.long_func_52:
	bne			a0,x0,.long_func_53
.long_func_54:
	addiw		t2,x0,1
	addiw		t1,x0,15
	bge			t2,t1,.long_func_80
.long_func_81:
	addiw		t1,x0,0
	bgt			t2,t1,.long_func_86
.long_func_87:
	add			t3,a1,x0
	add			t1,a1,x0
.long_func_88:
.long_func_82:
	add			a0,t0,x0
	add			a1,t1,x0
	add			a2,a3,x0
	add			t0,t1,x0
	jal			x0,.long_func_13
.long_func_86:
	lui			t1,8
	addiw		t1,t1,-1
	bgt			a1,t1,.long_func_89
.long_func_90:
	add			t1,t2,x0
	lui			t3,%hi(SHIFT_TABLE)
	addi		t3,t3,%lo(SHIFT_TABLE)
	slli		t1,t1,2
	add			t1,t3,t1
	lw			t1,0(t1)
	divw		t3,a1,t1
	add			t1,a1,x0
	add			a0,t3,x0
.long_func_91:
	add			t3,t1,x0
	add			t1,a0,x0
	jal			x0,.long_func_88
.long_func_89:
	add			t1,t2,x0
	lui			t3,%hi(SHIFT_TABLE)
	addi		t3,t3,%lo(SHIFT_TABLE)
	slli		t1,t1,2
	add			t1,t3,t1
	lw			t1,0(t1)
	divw		t1,a1,t1
	lui			t3,16
	addw		t3,t1,t3
	addiw		a0,x0,15
	subw		a0,a0,t2
	addiw		a1,x0,1
	addw		a0,a0,a1
	lui			a1,%hi(SHIFT_TABLE)
	addi		a1,a1,%lo(SHIFT_TABLE)
	slli		a0,a0,2
	add			a0,a1,a0
	lw			a0,0(a0)
	subw		t3,t3,a0
	add			a0,t3,x0
	jal			x0,.long_func_91
.long_func_80:
	addiw		t1,x0,0
	blt			a1,t1,.long_func_83
.long_func_84:
	addiw		t1,x0,0
.long_func_85:
	add			t3,a1,x0
	jal			x0,.long_func_82
.long_func_83:
	lui			t1,16
	addiw		t1,t1,-1
	jal			x0,.long_func_85
.long_func_53:
	addiw		t1,x0,0
	addiw		t2,x0,0
	add			t3,a0,x0
	add			t6,a3,x0
	add			a2,t2,x0
.long_func_55:
	addiw		t2,x0,16
	blt			a2,t2,.long_func_56
.long_func_57:
	addiw		t2,x0,0
	addiw		t3,x0,0
	add			t6,a0,x0
	add			a0,a3,x0
	add			a2,t3,x0
.long_func_65:
	addiw		t3,x0,16
	blt			a2,t3,.long_func_66
.long_func_67:
	addiw		t3,x0,1
	addiw		a3,x0,15
	bgt			t3,a3,.long_func_71
.long_func_72:
	addiw		t3,x0,0
	addiw		t6,x0,0
	addiw		a0,x0,1
	lui			a2,%hi(SHIFT_TABLE)
	addi		a2,a2,%lo(SHIFT_TABLE)
	slli		a0,a0,2
	add			a0,a2,a0
	lw			a0,0(a0)
	mulw		t2,t2,a0
	lui			a0,16
	addiw		a0,a0,-1
	add			a2,t2,x0
	add			t2,t3,x0
.long_func_74:
	addiw		t3,x0,16
	blt			t6,t3,.long_func_75
.long_func_76:
	add			t3,a0,x0
	add			a4,a2,x0
.long_func_73:
	add			a3,t1,x0
	add			a0,t1,x0
	add			a0,t2,x0
	add			a2,t3,x0
	add			t3,a4,x0
	add			t1,t2,x0
	jal			x0,.long_func_52
.long_func_75:
	addiw		t3,x0,2
	srliw		t3,a2,31
	add			t3,t3,a2
	andi		t3,t3,-2
	subw		t3,a2,t3
	bne			t3,x0,.long_func_79
.long_func_750:
	add			a3,t2,x0
.long_func_78:
	addiw		t2,x0,2
	srliw		t2,a2,31
	add			t2,a2,t2
	sraiw		t3,t2,1
	addiw		t2,x0,2
	srliw		t2,a0,31
	add			t2,a0,t2
	sraiw		t2,t2,1
	addiw		a0,x0,1
	addw		t6,t6,a0
	add			a0,t2,x0
	add			a2,t3,x0
	add			t2,a3,x0
	jal			x0,.long_func_74
.long_func_79:
	addiw		t3,x0,2
	srliw		t3,a0,31
	add			t3,t3,a0
	andi		t3,t3,-2
	subw		t3,a0,t3
	bne			t3,x0,.long_func_77
.long_func_751:
	add			a3,t2,x0
	jal			x0,.long_func_78
.long_func_77:
	addiw		t3,x0,1
	add			t3,t6,x0
	lui			a3,%hi(SHIFT_TABLE)
	addi		a3,a3,%lo(SHIFT_TABLE)
	slli		t3,t3,2
	add			t3,a3,t3
	lw			t3,0(t3)
	addw		t2,t2,t3
	add			a3,t2,x0
	jal			x0,.long_func_78
.long_func_71:
	addiw		t2,x0,0
	add			t3,t6,x0
	add			a4,a0,x0
	add			t6,a2,x0
	jal			x0,.long_func_73
.long_func_66:
	addiw		t3,x0,2
	srliw		t3,a0,31
	add			t3,t3,a0
	andi		t3,t3,-2
	subw		t3,a0,t3
	bne			t3,x0,.long_func_70
.long_func_748:
	add			a3,t2,x0
.long_func_69:
	addiw		t2,x0,2
	srliw		t2,a0,31
	add			t2,a0,t2
	sraiw		t3,t2,1
	addiw		t2,x0,2
	srliw		t2,t6,31
	add			t2,t6,t2
	sraiw		t2,t2,1
	addiw		t6,x0,1
	addw		a2,a2,t6
	add			t6,t2,x0
	add			a0,t3,x0
	add			t2,a3,x0
	jal			x0,.long_func_65
.long_func_70:
	addiw		t3,x0,2
	srliw		t3,t6,31
	add			t3,t3,t6
	andi		t3,t3,-2
	subw		t3,t6,t3
	bne			t3,x0,.long_func_68
.long_func_749:
	add			a3,t2,x0
	jal			x0,.long_func_69
.long_func_68:
	addiw		t3,x0,1
	add			t3,a2,x0
	lui			a3,%hi(SHIFT_TABLE)
	addi		a3,a3,%lo(SHIFT_TABLE)
	slli		t3,t3,2
	add			t3,a3,t3
	lw			t3,0(t3)
	addw		t2,t2,t3
	add			a3,t2,x0
	jal			x0,.long_func_69
.long_func_56:
	addiw		t2,x0,2
	srliw		t2,t6,31
	add			t2,t2,t6
	andi		t2,t2,-2
	subw		t2,t6,t2
	bne			t2,x0,.long_func_58
.long_func_59:
	addiw		t2,x0,2
	srliw		t2,t3,31
	add			t2,t2,t3
	andi		t2,t2,-2
	subw		t2,t3,t2
	bne			t2,x0,.long_func_63
.long_func_747:
.long_func_64:
.long_func_60:
	addiw		t2,x0,2
	srliw		t2,t6,31
	add			t2,t6,t2
	sraiw		t6,t2,1
	addiw		t2,x0,2
	srliw		t2,t3,31
	add			t2,t3,t2
	sraiw		t2,t2,1
	addiw		t3,x0,1
	addw		a2,a2,t3
	add			t3,t2,x0
	jal			x0,.long_func_55
.long_func_63:
	addiw		t2,x0,1
	add			t2,a2,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,a4,t2
	lw			t2,0(t2)
	addw		t1,t1,t2
	jal			x0,.long_func_64
.long_func_58:
	addiw		t2,x0,2
	srliw		t2,t3,31
	add			t2,t2,t3
	andi		t2,t2,-2
	subw		t2,t3,t2
	addiw		a4,x0,0
	beq			t2,a4,.long_func_61
.long_func_746:
.long_func_62:
	jal			x0,.long_func_60
.long_func_61:
	addiw		t2,x0,1
	add			t2,a2,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,a4,t2
	lw			t2,0(t2)
	addw		t1,t1,t2
	jal			x0,.long_func_62
.long_func_22:
	addiw		t0,x0,0
	add			a3,a2,x0
	add			t0,t1,x0
.long_func_24:
	bne			a3,x0,.long_func_25
.long_func_26:
	add			t0,a0,x0
	add			t1,a0,x0
	jal			x0,.long_func_23
.long_func_25:
	addiw		t0,x0,0
	addiw		t1,x0,0
	add			t2,a3,x0
	add			t3,a0,x0
	add			t6,t1,x0
.long_func_27:
	addiw		t1,x0,16
	blt			t6,t1,.long_func_28
.long_func_29:
	addiw		t1,x0,0
	addiw		t2,x0,0
	add			t3,a3,x0
	add			t6,a0,x0
	add			a0,t2,x0
.long_func_37:
	addiw		t2,x0,16
	blt			a0,t2,.long_func_38
.long_func_39:
	addiw		t2,x0,1
	addiw		a3,x0,15
	bgt			t2,a3,.long_func_43
.long_func_44:
	addiw		t2,x0,0
	addiw		t3,x0,0
	addiw		t6,x0,1
	lui			a0,%hi(SHIFT_TABLE)
	addi		a0,a0,%lo(SHIFT_TABLE)
	slli		t6,t6,2
	add			t6,a0,t6
	lw			t6,0(t6)
	mulw		t1,t1,t6
	lui			t6,16
	addiw		t6,t6,-1
	add			a0,t1,x0
	add			a3,t3,x0
	add			t1,t2,x0
.long_func_46:
	addiw		t2,x0,16
	blt			a3,t2,.long_func_47
.long_func_48:
	add			t2,t6,x0
	add			t3,a0,x0
	add			t6,a3,x0
.long_func_45:
	add			a0,t0,x0
	add			a3,t1,x0
	add			t0,t1,x0
	jal			x0,.long_func_24
.long_func_47:
	addiw		t2,x0,2
	srliw		t2,a0,31
	add			t2,t2,a0
	andi		t2,t2,-2
	subw		t2,a0,t2
	bne			t2,x0,.long_func_51
.long_func_744:
	add			a4,t1,x0
.long_func_50:
	addiw		t1,x0,2
	srliw		t1,a0,31
	add			t1,a0,t1
	sraiw		t2,t1,1
	addiw		t1,x0,2
	srliw		t1,t6,31
	add			t1,t6,t1
	sraiw		t1,t1,1
	addiw		t3,x0,1
	addw		t3,a3,t3
	add			t6,t1,x0
	add			a0,t2,x0
	add			a3,t3,x0
	add			t1,a4,x0
	jal			x0,.long_func_46
.long_func_51:
	addiw		t2,x0,2
	srliw		t2,t6,31
	add			t2,t2,t6
	andi		t2,t2,-2
	subw		t2,t6,t2
	bne			t2,x0,.long_func_49
.long_func_745:
	add			a4,t1,x0
	jal			x0,.long_func_50
.long_func_49:
	addiw		t2,x0,1
	add			t2,a3,x0
	lui			t3,%hi(SHIFT_TABLE)
	addi		t3,t3,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,t3,t2
	lw			t2,0(t2)
	addw		t1,t1,t2
	add			a4,t1,x0
	jal			x0,.long_func_50
.long_func_43:
	addiw		t1,x0,0
	add			t2,t3,x0
	add			t3,t6,x0
	add			t6,a0,x0
	jal			x0,.long_func_45
.long_func_38:
	addiw		t2,x0,2
	srliw		t2,t6,31
	add			t2,t2,t6
	andi		t2,t2,-2
	subw		t2,t6,t2
	bne			t2,x0,.long_func_42
.long_func_742:
	add			a3,t1,x0
.long_func_41:
	addiw		t1,x0,2
	srliw		t1,t6,31
	add			t1,t6,t1
	sraiw		t2,t1,1
	addiw		t1,x0,2
	srliw		t1,t3,31
	add			t1,t3,t1
	sraiw		t1,t1,1
	addiw		t3,x0,1
	addw		a0,a0,t3
	add			t3,t1,x0
	add			t6,t2,x0
	add			t1,a3,x0
	jal			x0,.long_func_37
.long_func_42:
	addiw		t2,x0,2
	srliw		t2,t3,31
	add			t2,t2,t3
	andi		t2,t2,-2
	subw		t2,t3,t2
	bne			t2,x0,.long_func_40
.long_func_743:
	add			a3,t1,x0
	jal			x0,.long_func_41
.long_func_40:
	addiw		t2,x0,1
	add			t2,a0,x0
	lui			a3,%hi(SHIFT_TABLE)
	addi		a3,a3,%lo(SHIFT_TABLE)
	slli		t2,t2,2
	add			t2,a3,t2
	lw			t2,0(t2)
	addw		t1,t1,t2
	add			a3,t1,x0
	jal			x0,.long_func_41
.long_func_28:
	addiw		t1,x0,2
	srliw		t1,t3,31
	add			t1,t1,t3
	andi		t1,t1,-2
	subw		t1,t3,t1
	bne			t1,x0,.long_func_30
.long_func_31:
	addiw		t1,x0,2
	srliw		t1,t2,31
	add			t1,t1,t2
	andi		t1,t1,-2
	subw		t1,t2,t1
	bne			t1,x0,.long_func_35
.long_func_741:
.long_func_36:
.long_func_32:
	addiw		t1,x0,2
	srliw		t1,t3,31
	add			t1,t3,t1
	sraiw		t3,t1,1
	addiw		t1,x0,2
	srliw		t1,t2,31
	add			t1,t2,t1
	sraiw		t1,t1,1
	addiw		t2,x0,1
	addw		t6,t6,t2
	add			t2,t1,x0
	jal			x0,.long_func_27
.long_func_35:
	addiw		t1,x0,1
	add			t1,t6,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t1,t1,2
	add			t1,a4,t1
	lw			t1,0(t1)
	addw		t0,t0,t1
	jal			x0,.long_func_36
.long_func_30:
	addiw		t1,x0,2
	srliw		t1,t2,31
	add			t1,t1,t2
	andi		t1,t1,-2
	subw		t1,t2,t1
	addiw		a4,x0,0
	beq			t1,a4,.long_func_33
.long_func_740:
.long_func_34:
	jal			x0,.long_func_32
.long_func_33:
	addiw		t1,x0,1
	add			t1,t6,x0
	lui			a4,%hi(SHIFT_TABLE)
	addi		a4,a4,%lo(SHIFT_TABLE)
	slli		t1,t1,2
	add			t1,a4,t1
	lw			t1,0(t1)
	addw		t0,t0,t1
	jal			x0,.long_func_34
.long_func_17:
	addiw		t0,x0,2
	srliw		t0,t3,31
	add			t0,t0,t3
	andi		t0,t0,-2
	subw		t0,t3,t0
	bne			t0,x0,.long_func_21
.long_func_737:
	add			a3,t1,x0
.long_func_20:
	addiw		t0,x0,2
	srliw		t0,t3,31
	add			t0,t3,t0
	sraiw		t1,t0,1
	addiw		t0,x0,2
	srliw		t0,t2,31
	add			t0,t2,t0
	sraiw		t0,t0,1
	addiw		t2,x0,1
	addw		t6,t6,t2
	add			t2,t0,x0
	add			t3,t1,x0
	add			t1,a3,x0
	jal			x0,.long_func_16
.long_func_21:
	addiw		t0,x0,2
	srliw		t0,t2,31
	add			t0,t0,t2
	andi		t0,t0,-2
	subw		t0,t2,t0
	bne			t0,x0,.long_func_19
.long_func_738:
	add			a3,t1,x0
	jal			x0,.long_func_20
.long_func_19:
	addiw		t0,x0,1
	add			t0,t6,x0
	lui			a3,%hi(SHIFT_TABLE)
	addi		a3,a3,%lo(SHIFT_TABLE)
	slli		t0,t0,2
	add			t0,a3,t0
	lw			t0,0(t0)
	addw		t0,t1,t0
	add			a3,t0,x0
	jal			x0,.long_func_20
.long_func_6:
	addiw		t0,x0,2
	srliw		t0,t3,31
	add			t0,t0,t3
	andi		t0,t0,-2
	subw		t0,t3,t0
	bne			t0,x0,.long_func_10
.long_func_734:
	add			a0,t1,x0
.long_func_9:
	addiw		t0,x0,2
	srliw		t0,t3,31
	add			t0,t3,t0
	sraiw		t1,t0,1
	addiw		t0,x0,2
	srliw		t0,t2,31
	add			t0,t2,t0
	sraiw		t0,t0,1
	addiw		t2,x0,1
	addw		t6,t6,t2
	add			t2,t0,x0
	add			t3,t1,x0
	add			t1,a0,x0
	jal			x0,.long_func_5
.long_func_10:
	addiw		t0,x0,2
	srliw		t0,t2,31
	add			t0,t0,t2
	andi		t0,t0,-2
	subw		t0,t2,t0
	bne			t0,x0,.long_func_8
.long_func_735:
	add			a0,t1,x0
	jal			x0,.long_func_9
.long_func_8:
	addiw		t0,x0,1
	add			t0,t6,x0
	lui			a0,%hi(SHIFT_TABLE)
	addi		a0,a0,%lo(SHIFT_TABLE)
	slli		t0,t0,2
	add			t0,a0,t0
	lw			t0,0(t0)
	addw		t0,t1,t0
	add			a0,t0,x0
	jal			x0,.long_func_9
main:
.main_0:
	sd			ra,-8(sp)
	addi		sp,sp,-16
.main_1:
	call		long_func
	add			t0,a0,x0
	add			a0,t0,x0
	addi		sp,sp,16
	ld			ra,-8(sp)
	jalr		x0,ra,0
	.data
SHIFT_TABLE:
	.word	1
	.word	2
	.word	4
	.word	8
	.word	16
	.word	32
	.word	64
	.word	128
	.word	256
	.word	512
	.word	1024
	.word	2048
	.word	4096
	.word	8192
	.word	16384
	.word	32768
