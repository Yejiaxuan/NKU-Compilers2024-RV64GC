declare i32 @getint()
declare i32 @getch()
declare float @getfloat()
declare i32 @getarray(ptr)
declare i32 @getfarray(ptr)
declare void @putint(i32)
declare void @putch(i32)
declare void @putfloat(float)
declare void @putarray(i32,ptr)
declare void @putfarray(i32,ptr)
declare void @_sysy_starttime(i32)
declare void @_sysy_stoptime(i32)
declare void @llvm.memset.p0.i32(ptr,i8,i32,i1)
declare i32 @llvm.umax.i32(i32,i32)
declare i32 @llvm.umin.i32(i32,i32)
declare i32 @llvm.smax.i32(i32,i32)
declare i32 @llvm.smin.i32(i32,i32)
define i32 @main()
{
L0:  ;
    br label %L1
L1:  ;
    %r1 = call i32 @getint()
    %r3 = call i32 @getint()
    %r5 = add i32 0,0
    %r7 = add i32 0,0
    br label %L2
L2:  ;
    %r1304 = phi i32 [%r7,%L1],[%r1284,%L6]
    %r1303 = phi i32 [%r5,%L1],[%r64,%L6]
    %r50 = icmp slt i32 %r1304,%r3
    br i1 %r50, label %L3, label %L4
L3:  ;
    %r53 = add i32 %r1303,%r1304
    %r55 = add i32 4,0
    %r56 = sdiv i32 %r1304,%r55
    %r57 = add i32 %r53,%r56
    %r59 = add i32 10,0
    %r60 = sdiv i32 %r1304,%r59
    %r61 = add i32 %r57,%r60
    %r63 = add i32 998244353,0
    %r64 = srem i32 %r61,%r63
    %r1146 = add i32 2,0
    %r1147 = srem i32 %r1304,%r1146
    %r1148 = add i32 0,0
    %r1149 = icmp eq i32 %r1147,%r1148
    br i1 %r1149, label %L5, label %L6
L4:  ;
    call void @putint(i32 %r1303)
    %r1301 = add i32 10,0
    call void @putch(i32 %r1301)
    %r1302 = add i32 0,0
    ret i32 %r1302
L5:  ;
    br label %L6
L6:  ;
    %r1283 = add i32 1,0
    %r1284 = add i32 %r1304,%r1283
    br label %L2
}
