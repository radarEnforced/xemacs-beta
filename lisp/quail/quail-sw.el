(require 'quail)
;; # !Id: SW.tit,v 1.1 1991/10/27 06:21:16 ygz Exp !
;; # HANZI input table for cxterm
;; # To be used by cxterm, convert me to .cit format first
;; # .cit version 1
;; ENCODE:	GB
;; MULTICHOICE:	YES
;; PROMPT:	$A::WVJdHk!KJWN2!K# (B
;; #
;; COMMENT	($AT4SZ(B CCDOS)
;; COMMENT	$AJiP48C::WVJ15D!8JW1J!9<0!8N21J!9!#@}Hg#,!>B@!?JWN21J=TN*!8?Z!9#,9JTZ(B
;; COMMENT $A!8JWN2!9D#J=VPSC(B ff0 $AH}<|JdHk!##((B f $A<|TZ!8JWN2!9D#J=VP1mJ>!8?Z!9#)(B
;; # define keys
;; VALIDINPUTKEY:	abcdefghijklmnopqrstuvwxyz
;; SELECTKEY:	1\040
;; SELECTKEY:	2
;; SELECTKEY:	3
;; SELECTKEY:	4
;; SELECTKEY:	5
;; SELECTKEY:	6
;; SELECTKEY:	7
;; SELECTKEY:	8
;; SELECTKEY:	9
;; SELECTKEY:	0
;; BACKSPACE:	\010\177
;; DELETEALL:	\015\025
;; MOVERIGHT:	.>
;; MOVELEFT:	,<
;; REPEATKEY:	\020\022
;; KEYPROMPT(a):	$APDSV(B
;; KEYPROMPT(b):	$AZ"I=(B
;; KEYPROMPT(c):	$AJ,MA(B
;; KEYPROMPT(d):	$AX<56(B
;; KEYPROMPT(e):	$A;pZb(B
;; KEYPROMPT(f):	$A?Z?Z(B
;; KEYPROMPT(g):	$A^PRB(B
;; KEYPROMPT(h):	$Ac_qb(B
;; KEYPROMPT(i):	$AZ%4s(B
;; KEYPROMPT(j):	$A\36!(B
;; KEYPROMPT(k):	$AXi[L(B
;; KEYPROMPT(l):	$AD>la(B
;; KEYPROMPT(m):	$AljJ.(B
;; KEYPROMPT(n):	$Ab;4u(B
;; KEYPROMPT(o):	$ATBXg(B
;; KEYPROMPT(p):	$AfyCE(B
;; KEYPROMPT(q):	$AJ/=q(B
;; KEYPROMPT(r):	$AMuX-(B
;; KEYPROMPT(s):	$A0KE.(B
;; KEYPROMPT(t):	$AX/RR(B
;; KEYPROMPT(u):	$AHU`m(B
;; KEYPROMPT(v):	$AeAP!(B
;; KEYPROMPT(w):	$Aak3'(B
;; KEYPROMPT(x):	$AVq3f(B
;; KEYPROMPT(y):	$AR;_.(B
;; KEYPROMPT(z):	$AHK27(B
;; # the following line must not be removed
;; BEGINDICTIONARY
(quail-define-package "sw" "$AJWN2(B"
 t
 "$A::WVJdHk!KJWN2!K# (B
	($AT4SZ(B CCDOS)
	$AJiP48C::WVJ15D!8JW1J!9<0!8N21J!9!#@}Hg#,!>B@!?JWN21J=TN*!8?Z!9#,9JTZ(B
 $A!8JWN2!9D#J=VPSC(B ff0 $AH}<|JdHk!##((B f $A<|TZ!8JWN2!9D#J=VP1mJ>!8?Z!9#)(B"
 '(
  ("." . quail-next-candidate-block)
  (">" . quail-next-candidate-block)
  ("," . quail-prev-candidate-block)
  ("<" . quail-prev-candidate-block)
  (" " . quail-select-current)
  )
 nil nil)

;; #
(qdv "aa"	"$A2f:6B}IeK+PDSVb`bec*c1c9m!(B")
(qdv "ac"	"$A6.9VDQJ%N)PJPTc%c&c?(B")
(qdv "ad"	"$A2@<&A/H0Lhbabvbzb~c)c5c6(B")
(qdv "ae"	"$A5K;Vc8(B")
(qdv "af"	"$A5kG!LqNrbobwbyc!(B")
(qdv "ag"	"$A:^<hbj(B")
(qd "ah"	"$Ac@(B")
(qdv "ai"	"$A0C?lbsb{(B")
(qdv "aj"	"$A6T<BJQSdbbb|c#(B")
(qdv "ak"	"$AGSbl(B")
(qdv "al"	"$AI#bpc>(B")
(qdv "am"	"$A5,5?:7P8bcbhbqbxc"c,c2(B")
(qdv "ao"	"$A626hDUE3GDGibfc+c.c/c0c7c<(B")
(qd "ap"	"$ACu(B")
(qdv "ar"	"$A2Q;LPCbg(B")
(qdv "as"	"$A>eIw(B")
(qdv "at"	"$A1X2R3@9[;E;P?.@"MoRdSGTCbdbkbub}c((B")
(qdv "au"	"$A;ZEBO'Twc'c;cB(B")
(qdv "av"	"$A;3>*?6c$c:c=cA(B")
(qd "ax"	"$ATi(B")
(qdv "ay"	"$A5~:cC&O7PtUzbibnbrc3(B")
(qdv "az"	"$A7_9_;6@Abmbtc-c4r*(B")
(qdv "ba"	"$A<E>~G^a*aLe5qd(B")
(qdv "bb"	"$A3v?_C\I=R$Z"aH(B")
(qdv "bc"	"$A4^A~H{JRLCQBVO(B")
(qdv "bd"	"$A3#9Q:.:WA1AkC]GOGTGnP4Sla8a>a@e9p2q7q:(B")
(qdv "be"	"$AL?TV[)a9(B")
(qdv "bf"	"$A9,9Y:&;m<R=Q>=?MH]SxZ#`7a2e;e@(B")
(qdv "bg"	"$AE)IQe7e=e>(B")
(qdv "bh"	"$A6(aZe?(B")
(qdv "bi"	"$AD/J5M;O?q<(B")
(qdv "bj"	"$A4)8n<DD~FiJXQ'SnUFVEWVX[X\X`a,a3a?aFaNe:q>(B")
(qdv "bk"	"$A3":jaBaQ(B")
(qdv "bl"	"$A08CBKNLDU/aAaIq=tP(B")
(qdv "bm"	"$A067e:1>|@NTWa)a;aVaW(B")
(qdv "bn"	"$AKjq6(B")
(qdv "bo"	"$A1@8Z;B?yGMNQO|T"a'a+a0a<aEaTaUe6e8(B")
(qdv "bq"	"$AQRa/e4(B")
(qdv "br"	"$A1&4\IsU8a5a=ex(B")
(qdv "bs"	"$A021vQ(QgRzYdZ$aJaPq@(B")
(qdv "bt"	"$A3h539Z>?>u?-?\?m?zAHFqH_K|MjMpN!O\RYT)U,a(a-a1aCaMe3jiq8q;(B")
(qdv "bu"	"$A408;K^Vfa6aDaRq9q?qA(B")
(qdv "bv"	"$A2l3gKnWZa4aSe<m4(B")
(qdv "bx"	"$AC[r?(B")
(qdv "by"	"$A6k?UP{RKa:aGaKaOaXvW(B")
(qdv "bz"	"$A6nG6H|IMU-a[r'(B")
(qdv "ca"	"$A5n9D;[BDBsFBN?V>[`[~\,\2ell0l1l2m"m#m(tv(B")
(qdv "cb"	"$A;wG|\%s@(B")
(qdv "cc"	"$A1Z6Q9M9gBqIyJ,J?MAN][d[l\'qs(B")
(qdv "cd"	"$A3!3`7;7?:U=Y>!>V>y?<?eEUE|HMIHKzN$Nk[X[^[j[|\#\(fqpLqltctftp(B")
(qdv "ce"	"$A6<:BH4lY(B")
(qdv "cf"	"$A98:><*<N>SE`F)FtK~LAO2[cqjqkv%(B")
(qdv "cg"	"$A1mH@T,U9[stE(B")
(qdv "ch"	"$A3,3C5L5e808OFpGwH$LKT=UTW_eotqtrtstttu(B")
(qdv "ci"	"$A0#?iFu\)ialeto(B")
(qdv "cj"	"$A7b>2>g?@K"KBN>P"[W[u[xemenj|k"qhqq(B")
(qdv "ck"	"$A2cH%L3TE(B")
(qdv "cl"	"$A0>6bD)J:N4Vs\&\.i^ldqg(B")
(qdv "cm"	"$A1Y2:7a8{F:FAO,PRUX[}\/\1eqlfqz(B")
(qdv "cn"	"$AvKvM(B")
(qdv "co"	"$A0=1[1b6U7X9!;x<g=LFRG`IJIbJtL.MNP<Q_[m[p[q[v\!\*\-eglgqmqnqp(B")
(qdv "cq"	"$Am9m`qr(B")
(qdv "cr"	"$A@$[Z[_[th5(B")
(qdv "cs"	"$ABELnepfKfTqo(B")
(qdv "ct"	"$A0R5X:D>3?G?S@ODaEmF(N2Q^[Y[\[][b[g[h[n[y[{\"ejj4j6jkk#lhqiv&(B")
(qdv "cu"	"$A3_6>6B=lC<G=M@TvU_[a[i\0ekh:jHn-tw(B")
(qdv "cv"	"$A;5DrKX[o\$\+tltn(B")
(qd "cw"	"$A[[(B")
(qdv "cx"	"$Ar|s'(B")
(qdv "cy"	"$A3G:x?0@,EwL9LkPfQNR<SrT+V7Vg[f[k[zh9nAw!(B")
(qdv "cz"	"$A0S?2?nTpW8XT[e[r[wlir!(B")
(qdv "da"	"$A1d1o?"A5AhEQGCHLM|RbRcm'm-m0(B")
(qdv "db"	"$ABMe2tL(B")
(qdv "dc"	"$A5T;'M/R+S:W3W<[S[Ul:(B")
(qdv "dd"	"$A5[7=7[7k@dETH/HPJPMdN*R`SpTOX<Y{[?[@l6l7p=pHpRpUq4tOtT(B")
(qdv "de"	"$A2?9y=<@IDGI[ZxZz[5(B")
(qdv "df"	"$A:@;?A_IXK>LGL\QTR1S~U3U>UYWI`4(B")
(qdv "dg"	"$A0'0}9|A8A<BCK%OeRBVTW0YrYsYtYvwS(B")
(qdv "dh"	"$AD}P}u?(B")
(qdv "di"	"$A4U=1>v@`M7W4We^HtWt\(B")
(qdv "dj"	"$A:`<A=+>;?LAuBNBOEPFJH-M$OmOwWQY}l=(B")
(qdv "dk"	"$AP~Z!tY(B")
(qdv "dl"	"$A8b=0CWEkGWJlhohptRt[(B")
(qdv "dm"	"$A0j0k1W1f1g1h4b4d67;T>YAOBJFzPAUBWd^Dt`(B")
(qd "dn"	"$AvG(B")
(qdv "do"	"$A1V3(515r6K6X7E8_8`:}=;>+>,>8@J@kE4ILIPJ}NDP$P'R9RaReS}U+YukAkvl*l9tMtZ(B")
(qd "dp"	"$Al5(B")
(qdv "dr"	"$A3eFkM{PBQ>V]Vwc\l4(B")
(qdv "ds"	"$A7`AyB&F`FlM}PKRmW1WKf*f.(B")
(qdv "dt"	"$A3d4I565s9b:A>9>:>M>m?:?vAAJ)O0QeS.UCYpYqYxYyYzY~j3k)l8l;l<tNtV(B")
(qdv "du"	"$A=4=_>lC$D6F3FIH?PsRtTcV.jFtQtX(B")
(qdv "dv"	"$A1y2Z63=,>)?7@1A9A]NIS@Ywq5tU(B")
(qdv "dw"	"$A2z9cpZ(B")
(qdv "dx"	"$AByt](B")
(qdv "dy"	"$A1n1s4A4V5A<uA"A#B>C%JkMvN@RnYoY|v_(B")
(qdv "dz"	"$A1e2|4N8S:%QUWJl'q~r$(B")
(qdv "ea"	"$AAjO(R~ZfZiZn[FlQlW(B")
(qdv "eb"	"$A2SB=(B")
(qdv "ec"	"$A1]6iB!B/TnW9ZmZo(B")
(qdv "ed"	"$A7@=}?>I?LUO)WFfol>l?(B")
(qdv "ee"	"$A;pQWZ`Zb[0lM(B")
(qdv "ef"	"$A1:@SEcH[ZglIlJ(B")
(qdv "eg"	"$AO^ZqlP(B")
(qd "eh"	"$A68(B")
(qdv "ei"	"$A;@H2IBl[(B")
(qdv "ej"	"$A0"5F8=X_lCl_(B")
(qdv "ek"	"$A;blE(B")
(qdv "el"	"$A3}C:H<ToZt(B")
(qdv "em"	"$A7i:8=5UOUsZdZeZplGlHlL(B")
(qdv "eo"	"$A1~8t><>fKeRuSglNlTlX(B")
(qd "ep"	"$AlK(B")
(qd "eq"	"$AlB(B")
(qdv "er"	"$A;M=WZrlD(B")
(qdv "es"	"$A3c:f(B")
(qdv "et"	"$A34?;B$EZHnIUMSMiT:ZcZlZsl@lA(B")
(qdv "eu"	"$AD0O]QLQfQtlVlZl\(B")
(qdv "ev"	"$A1,3B<JA6AGK8KfKmO6cDlUl](B")
(qd "ex"	"$AVr(B")
(qdv "ey"	"$A0/@CB*L~OUWhZjlOlS(B")
(qdv "ez"	"$A466S73TIU(ZhZk(B")
(qdv "fa"	"$A0OKtL>V(_D_N_i_|`#`(`2`?`D`E`H`\`fuKuUubufus(B")
(qdv "fb"	"$A_M`W(B")
(qdv "fc"	"$AA(MBMYM[N(_r`,`-`8`g`kuMu`up(B")
(qdv "fd"	"$A5p5u7T:E>i?gAmBnBpCyKCL#L_LcLdNGNKNXQ+S4_6_7_?_J_K_O_j_k_q_y_{`OpJpXu@un(B")
(qdv "fe"	"$A1I6uDDTG[+_5_T_s`"`%`1`=l^u\(B")
(qdv "ff"	"$A9>9~:?>a?&?'?)?ZB7B@F7FwI6N{OyQdWD_R_m_v`*`9`e`mu9uJuLuXuZu[(B")
(qdv "fg"	"$A8zHBN9___f_g`luTwR(B")
(qdv "fh"	"$ALgWc`$(B")
(qdv "fi"	"$A0&5x7M:m;=?^NbPaT>_;_>`&`U`[`^uCuhuk(B")
(qdv "fj"	"$A0!1p3l6#6W8@9P:G:_:t=@@.OxQ=SuSw_L_V`'`@uFui(B")
(qdv "fk"	"$A_:_`(B")
(qdv "fl"	"$A2H4t6e:YBoI$N6TjTk_]_a_d_x`)`:`]uWu^ue(B")
(qdv "fm"	"$A;)F!R6_Y_c_h_t_}`F`huBuOuV(B")
(qdv "fn"	"$A6__\`G(B")
(qdv "fo"	"$A0%1D28304->`?PDEDvE;IZN|OlR'S;VvWl_C_F_Q_l_z`!`+`/`0`;`C`J`T`V`_`c`iuAuGuSu_uaudug(B")
(qd "fp"	"$Auo(B")
(qdv "fq"	"$ARwuE(B")
(qdv "fr"	"$A3J4Z=PIkK;L}P%_9_E_H_b_nuR(B")
(qdv "fs"	"$A0H:eE?V;_~`6`A`S(B")
(qdv "ft"	"$A0I333T6V9r:p;#?TA|DXEXE\G:K1LxPVRXVd_3_4_8_<_=_@_A_X_Z_[_^_e_p`XuDuIuNuPumv<v>(B")
(qdv "fu"	"$A2d3*3y588A:HE>JHL$QJT[_I_S_w`.`3`K`P```aul(B")
(qdv "fv"	"$AS=WY_U`<`B`L`Q`Z`d`juHuQu](B")
(qdv "fx"	"$AKd`M(B")
(qdv "fy"	"$A5E8B:0<y>W@2E6E^JIPjPzQFR-T{V:_P`5`>`I`R`bj+ucuq(B")
(qdv "fz"	"$A457H?HEgKTOET1U&_2_B_G_W_o_u`Yr&uYujur(B")
(qdv "ga"	"$A0N0a0b1(2&4i6^:3<<>pDmE{IcJZKQM6T.^s^t(B")
(qdv "gb"	"$A>rR!W>(B")
(qdv "gc"	"$A474]4l5f9R;$DsL/MFNUW2^__*_+(B")
(qdv "gd"	"$A0g2s577w9U>P?=?f@LA`B0HSJFLMP/Qo^R^V^d^yp:(B")
(qdv "ge"	"$A>>E2RVV@^^(B")
(qdv "gf"	"$A4n5`>]?[@(ALDiJ0JDL'LBNfUPU\^W^\^e^r(B")
(qdv "gg"	"$AHAUq^P^x_'_-(B")
(qdv "gh"	"$A=]LaW=u=(B")
(qdv "gi"	"$A0$7v;;>qC~P.Wa^f^q(B")
(qdv "gj"	"$A0F2+3E3V4rE!JcUuV?^T^[^a^m_$(B")
(qd "gk"	"$A0Z(B")
(qdv "gl"	"$A2Y2kD(HHH`L=^i^z^}(B")
(qdv "gm"	"$A0h5'5t66:4;SDlEjEuF4G$K$R>Tq^U^c^p_&_((B")
(qd "gn"	"$A^Y(B")
(qdv "go"	"$A26374'528c>\>h?YF2G\HvI(ISKSM1R4S5U*^g^k^w^~(B")
(qdv "gp"	"$A8i^Q(B")
(qd "gq"	"$AMX(B")
(qdv "gr"	"$A<7K)K:Q:U[Vt^S^X_%_)(B")
(qdv "gs"	"$A040G1w90=SB'W+^|(B")
(qdv "gt"	"$A0Q1'2t3-6s8'=A?9@?B#BUDSEWEzG@GKHELtMOMPMZMlQZTzV4^Z^u(B")
(qdv "gu"	"$A2%2e3i4$4k=R?+@&@^ChD4EDF~V8W%^b^n^v_!_#(B")
(qdv "gv"	"$A2A<pACBSL"M&NN^]^`^{_,tj(B")
(qd "gw"	"$A@)(B")
(qdv "gx"	"$AI&U]rX(B")
(qdv "gy"	"$A364j5#5V<q?8?X@-@9CrE$ICJCURU|W'^j^o(B")
(qdv "gz"	"$A2p>oDbEEFKKpOFT\^h^l_"j^(B")
(qdv "ha"	"$A2(6I::=~?#BKB~C;CZFCG_JgcbdQdSd\dqm/(B")
(qd "hb"	"$AIG(B")
(qdv "hc"	"$A9`;&;4DyL2M]N+QDRycrdNdWd|e!e*(B")
(qdv "hd"	"$A237P7Z95:h@TA$DgEfFcL@LTLiMeN[P:SYVMcfcmcncvd-d;dbdhe+(B")
(qdv "he"	"$A5-LLcwdP(B")
(qdv "hf"	"$A8"9A:F;n=`B:BeG"H\QXT!U4USVNd2d4dCdJdge#e)(B")
(qdv "hg"	"$A9v@KE(EIUGtD(B")
(qdv "hh"	"$A5mc_d7dv(B")
(qdv "hi"	"$A0D;AD.L-NVO*csd$dBdee'(B")
(qdv "hj"	"$A2b4>8!:SE"LNM!SNSed#d/d1dLdUd_dadodtdxd}j}(B")
(qdv "hk"	"$A7(WLcyc|d+dA(B")
(qdv "hl"	"$A3|5SA:A;A\BPD-FbG~H>InM?Thcecpd(d5dMdZd`(B")
(qdv "hm"	"$A4c6}:9;k=rDWEHL6QsTsUDV-WRccczd.d0d](B")
(qdv "hn"	"$AO+dFdfvHvL(B")
(qdv "ho"	"$A313:5N64:-:~;,<$<3@lB)BzE=FVGeK]LJMDN<NPO{O}PZR:S?ckd"d8drdwdzd{e&e.e1(B")
(qdv "hp"	"$A='@=Hs(B")
(qd "hq"	"$Ad9(B")
(qdv "hr"	"$A<C=%MtRJT(UcV^W"d@dDdIdRd^dldye((B")
(qdv "hs"	"$A1u5a:iFEHjQ]d:d?didjdme/f6(B")
(qdv "ht"	"$A2W3A3X8H8[;l==AwBYClD`E]ElI3IfIrIxO4QMc`cacdcgclcqcuc{d,d<d=dEdGdYdddue-(B")
(qdv "hu"	"$A247::#:T>F?J@aAoEKG1GvHwLOOfSMWMchcicoctd&d'd)d6d>dOdXdcdndse%(B")
(qdv "hv"	"$AA0AJF/FYFaLmOQQzS>T4cxd3dHdKdkdp(B")
(qd "hx"	"$AWG(B")
(qdv "hy"	"$A3N;c=&=->Z@DF{F|G3I,J*NBP9Q4RgSfT|U?c}c~d!d*dTdVd[e"e$e,vh(B")
(qdv "hz"	"$A1t@#J~WUcjd%d~e0(B")
(qdv "ia"	"$AC!IhL,ZAZEZNZU(B")
(qdv "ib"	"$AR%Z(Z0(B")
(qdv "ic"	"$A=w?|K-Z4(B")
(qdv "id"	"$A0y2w4J7C:;;d?dL+Q/SUZ<ZLZPZYpFpS(B")
(qdv "ie"	"$AL8Z6(B")
(qdv "if"	"$A;0E5SoZ,Z/Z1Z5Z8Z>ZBZQZ^(B")
(qdv "ii"	"$A4s6A>wH.NsZ%Z@ZSZ](B")
(qdv "ij"	"$A6)6aFfJ+LVP;QHW;XZX^Z-Z:ZM(B")
(qdv "ik"	"$AKOLW(B")
(qdv "il"	"$A5}?ND1VoZ3Z[(B")
(qdv "im"	"$A1<<F=2F@G#L7OjPmRkZ&Z;ZG^G(B")
(qdv "io"	"$A5w7mGkKPN=RiV_Z)Z*Z+Z2Z=ZJZRZXZ\^Fl-(B")
(qd "ip"	"$A@>(B")
(qdv "ir"	"$AQ5Z9Z?^E(B")
(qdv "is"	"$AJ6ZC(B")
(qdv "it"	"$A2o6o9n;Q<%<GB[C}F}K5QYQhUoZ7ZT^K(B")
(qdv "iu"	"$A7\;eFWG+I]P3RhVnZFZKZOZZp-(B")
(qdv "iv"	"$A5.ABCUDNG4ZI(B")
(qdv "iy"	"$A3O=kHCJTN\Q6RjV$WgZ'Z.ZHZVZWZ_(B")
(qdv "iz"	"$A7L8<8CHOK_U)ZDl%(B")
(qdv "ja"	"$A2$4PAbB{HGHoP>Yg\A\O\X]C]D]J]_]g]l^%^)^2(B")
(qd "jb"	"$AWB(B")
(qdv "jc"	"$A2g6-B+D9P,\6\]\c\s]@]H^3^7^=(B")
(qdv "jd"	"$A545Y7<7R96=Z>O>U@M@U@sD;D<DhFOGZH5IVKULQN-R)\5\:\@\B\L\_\`\w]#],]:]N]`]k]n]rpEpYw6(B")
(qdv "je"	"$AS+[4\a\b]6(B")
(qdv "jf"	"$A9=>/?`BdCIFPGQHXHcHtI;L&S*\f\l\x]F]Q]m(B")
(qdv "jg"	"$AKr\I]"]9]e]u(B")
(qd "jh"	"$A]{(B")
(qdv "ji"	"$A;q<T?{D*S"\=\r]$]V]p]}w1(B")
(qdv "jj"	"$A1!:I:J<;<v=/>#?A@rC)D!DuGfN5Q?SsXaXe\3\^\t]!]'])]*]3]W]c]j^6^?w5(B")
(qdv "jk"	"$A\?\v]?(B")
(qdv "jl"	"$A1=2K2h=6@3HYQ`TeU:UaUt\T\o]1]a]h^!^9^;^AiQw7(B")
(qdv "jm"	"$A2]8o;g>.C'F;F<HWQ&Ta\7\j\}]7]I]M]]^&^5^:(B")
(qdv "jn"	"$AB\]v(B")
(qdv "jo"	"$A0,1N1^:y>4CHE:FNFQGJI"Rq\8\D\G\[\\\m\z]+]/]<]U]X]o]q]t]|^1^4^8w8w9(B")
(qd "jp"	"$A]~(B")
(qdv "jq"	"$AD"\K]P(B")
(qdv "jr"	"$A=f=yG[OtP=S(\q\u\y]>^-w3(B")
(qdv "js"	"$A0092;FN.]4]=]B]d^.(B")
(qdv "jt"	"$A0E0P0z1M2T76;(;DCjI/J_N_OoQ%RUT7\4\9\;\<\>\C\E\H\R\W\i\k\{]5]8];]K]R]b]y]z^"^$^0jmjnw4(B")
(qdv "ju"	"$A0*2\7*8p=e>z@YCgD:G>JmNtPnRpV%Vx\U\Y\g\n].]E]L]T]Y]\]i]s^#^(^+^,m3(B")
(qdv "jv"	"$A2L9'A+D=EnK.KbLY\p\~]%]A]S^<^>^@m5tkw2(B")
(qd "jw"	"$AHx(B")
(qdv "jx"	"$A<kS)(B")
(qdv "jy"	"$A2X>%@6C"C#C/CoTLX%\F\N\Q\V\Z\d\e\|]&]-]0]2]O]Z][]f^*^/w0(B")
(qdv "jz"	"$A4D7FH'\J\M\P\S\h](]G]^]w]x^'^Br)(B")
(qdv "ka"	"$A1K3M5!5B<?<Y=v?!DzGVI5R[SFYLaam%m)qe(B")
(qdv "kb"	"$A>sOIa7af(B")
(qdv "kc"	"$A1$4_<Q@]HNJKV6Y5YPYW[Rv?vE(B")
(qdv "kd"	"$A0x394+7B7]7pAfHTIKKEN0N1XlXpX~Y!Y(Y7a]a_p<pTvA(B")
(qdv "ke"	"$A;oQvSy["lFlR(B")
(qdv "kf"	"$A169@BBJ[KWL(OqPESSY$Y%Y<YDYFYRYY(B")
(qdv "kg"	"$A4|:\R@XvY/YKYX(B")
(qdv "kh"	"$A4YM=ac(B")
(qdv "ki"	"$A7|:n:rO@RSX}Y6Y9Y?(B")
(qdv "kj"	"$A2`4}595C7}8)8586:N:b=V@}@~JLM#M5ONPPQCQ\RPWPXdXjXsY1ai(B")
(qdv "kk"	"$A3%?kXiXo[La\(B")
(qdv "kl"	"$A1#</=9@|K[LeP\P]PlY*Y@YUabwl(B")
(qdv "km"	"$A0i;*<~BID2F'G*J2QpXuY&Y0Y:Y>YB[Ma`jpv@(B")
(qd "kn"	"$A3^(B")
(qdv "ko"	"$A0A1c61;UA)E<EeF+GNHeJ9LHN"RGS6ULWvXqX|Y'Y+Y.Y3Y8Y;YCYHYSadagah(B")
(qd "kp"	"$ACG(B")
(qdv "kr"	"$A<[GHIlMyVYW!Y-ae(B")
(qdv "ks"	"$A9)>cXzYAYMYO(B")
(qdv "kt"	"$A032N3p;/=D>k?~@PBWD\D_F>K{P^RARZSETJXkXnXrXtXwXxY"Y#Y)Y,YNYVk'(B")
(qdv "ku"	"$A0[2.3+4"5h;2=h@\I.NjQ-YIYJYT[N(B")
(qdv "kv"	"$A4v=!AEDcY4YQ(B")
(qd "kw"	"$AY2(B")
(qdv "ky"	"$A4z5+5M6m7%<s=)>6HJH~N;NiUwV5WtXmXyYEa^(B")
(qdv "kz"	"$A4{;uA^EGFMGcKFU.UlWwX{Y=YG(B")
(qdv "la"	"$A0e3n=7@bCXH(KkKsNHOkV&h>i2m.o~p"p%(B")
(qd "lb"	"$Aoz(B")
(qdv "lc"	"$A6E9pH6VIW.W5hSh_hdi"iHiL(B")
(qdv "ld"	"$A0q99@cC^JAL]O!P`PcQnh<h?h@hJhNhUhZh[h`hrh{i=iKov(B")
(qdv "le"	"$A0p3;7Y@FAxGoR,hyi1(B")
(qdv "lf"	"$A4*8q9W:M<O<Z=U=[?]CJN`OpPShihti'i/iEiZi\oyp$(B")
(qdv "lg"	"$A8yhGiAo|p&(B")
(qdv "lh"	"$A3~iP(B")
(qdv "li"	"$A7.D#P(QmVHh|i((B")
(qdv "lj"	"$A2D3w4e<>?B@n@{D{EJJwRNS\hLhlhui$i?iWo{(B")
(qdv "lk"	"$AK=KIhm(B")
(qdv "ll"	"$A1>=\?C@fAVD>I-JuVji4i;iTlaowoxo}ws(B")
(qdv "lm"	"$A0^0t3S8K8Q?F@gQyUAhFhRhkhwh~i.i7i>i@iNiX(B")
(qdv "ln"	"$A;`CNRFi!(B")
(qdv "lo"	"$A1z3m557c8e9#9q<+?rC6EoIRITJ`M)M0MVO-P#U$UHhDhQhvi*i<iBiGiOiUiY(B")
(qd "lp"	"$Ai5(B")
(qd "lq"	"$AhO(B")
(qdv "lr"	"$A3LGEK(MwNvVVVyhT(B")
(qdv "ls"	"$A:a;}@7B%FeN/S#hWhqiDp!(B")
(qdv "lt"	"$A1r8E9w:<;1;zB4CkDBG9GAI<JaK0LRM:N&N:T}Umh;h=hAhBhEhKhPhYh^hahchfhghshxi-ou(B")
(qdv "lu"	"$A0X2[4;5>8L;|?,AqC7F\O`OchChVhjhzi9iFiIiVi[p#p*(B")
(qdv "lv"	"$A1-1j3F60={@hJrWXhMh]hhi#i%i&i,i3i]i_(B")
(qdv "ly"	"$A2i3H8\;8<l<w@8L4P5U;V2Wbh\hbi)i+i6i8i:iJ(B")
(qdv "lz"	"$A:KFSMGU%WuhHhXheiS(B")
(qdv "ma"	"$A1;3a:d;]SQV'XE[Dj!lpqCqV(B")
(qdv "mc"	"$A;yB"G5IgP[TZ[TiqiyqE(B")
(qdv "md"	"$A0l0o2*2<3u4x:+:2<]?1A&GPP-TfW(W*imltp/p3p4p>pIqS(B")
(qdv "me"	"$A0n:*;RFnSt[#(B")
(qdv "mf"	"$A9E<SH9O=SRT#ioiwi{j&lol{qJqKqUqW(B")
(qdv "mg"	"$ADRI%O.T/U7l|qLqQtBtC(B")
(qdv "mi"	"$A0@7r8$<PBtOWW`isj#lllyqG(B")
(qdv "mj"	"$A2)2E3D4L4f5;HlJBJYJdXCX][Aipiuqa(B")
(qd "mk"	"$Aln(B")
(qdv "ml"	"$A<\@4BcD3GXM`TSh}iRlblzqY(B")
(qdv "mm"	"$A357n9<;\<-?cA{J.NSOiPyUeX&i}j%ljlxqBqHqT(B")
(qd "mn"	"$AvI(B")
(qdv "mo"	"$A3/7s8&8(9J:z;v=O>H@tA>DOFZSPUIU^i~k7qDqIqXq[q`(B")
(qd "mp"	"$AqP(B")
(qdv "mq"	"$AmCqF(B")
(qdv "mr"	"$A=NFmIqK9U6izqN(B")
(qdv "ms"	"$A9(F^FdUfirlwqZ(B")
(qdv "mt"	"$A9f9l9u>$>E?K@qAzBVE[F_G,I@JSMMMhPqR2SHT~W#X8^L^M^N^Oiniti|j$lklvq](B")
(qdv "mu"	"$A4:7x8#8J:VLfPdT]VaXDj"j8qRq^t)(B")
(qdv "mv"	"$A6+<,<m=sB;GsJxKwL)MJivlrqM(B")
(qdv "mx"	"$A4@q_s<(B")
(qdv "my"	"$A2C4#4w=X?xGaIuL;TTTUTXU{V1WfWsixj*j,lmlsqOq\(B")
(qdv "mz"	"$A295_6Y9S:X<U@5F[HmK,RDjZjcjelqlu(B")
(qdv "na"	"$A196F6M79<1BxDwHzT9VebHnNnSn`ninko7oAoHoKoLo\olonvQvlvmvsv{w)(B")
(qd "nb"	"$Aw%(B")
(qdv "nc"	"$A3{48@pW6b?bKnJo,o.o1o[oqvCvTvYvd(B")
(qdv "nd"	"$A0w2v3z5v6,8F93=u>{AeJNKGL`NYNqN}O&PbT'b<bAnUn_nfnmnvo&o)o8oIoTomu|vPvyw#w&(B")
(qdv "ne"	"$A>DC-GBVKW^nXntoDvjvz(B")
(qdv "nf"	"$A8u8w9]>L>bBACzC{G&OsU2WjbBehn\n~o(o/o;oBoXvSvXvZw-(B")
(qdv "ng"	"$AObR?Rxb8bNo6oM(B")
(qdv "nh"	"$A6'o`(B")
(qdv "ni"	"$AC>Lz[<b@bInQnroFoRo_odu{(B")
(qdv "nj"	"$A6$U!UyV}bDnHnIn]o#o2o9o=oOoUvVv`vew.(B")
(qdv "nk"	"$Angn{(B")
(qdv "nl"	"$A2'DxH;I7LubEhnnyo!o*o>opvUvfvx(B")
(qdv "nm"	"$A1}6|7f=bA[G%OJP?RMUkb6nWnlnon|o<obuzv#vR(B")
(qdv "nn"	"$A6`=pB`b;nDvN(B")
(qdv "no"	"$A8V8d9x=B=G=HDFFLM-OzT?[;bCbLnRn^npnuo:oSo^oaofv[v\v^vavbvkvtv}w(w+(B")
(qdv "np"	"$AnMo4o5og(B")
(qd "nq"	"$AnT(B")
(qdv "nr"	"$A<XVSnKnZn}o-oiv]v|(B")
(qdv "ns"	"$ADYUrb9bMnOo'oNoY(B")
(qdv "nt"	"$A1%1+3.6[709j<">5CbCcHDHqI+MCN#O3b=bFeinEnFnLnVnYnhnjnnnsnxo"o$o%o3o?oCv!vOvivovr(B")
(qdv "nu"	"$A182,4&4m>C@XA-AsB3C*G/OZSKU`b:jCmsnbndnwo@oEoGoJoQoVoeohu}vpvqvvw"(B")
(qdv "nv"	"$A<@<|>(A4AMnPnenzoZo]oov"vcvgw$w'(B")
(qd "nw"	"$A2y(B")
(qdv "nx"	"$A4%J4P7ojs.s;v~(B")
(qdv "ny"	"$A6v=$CLE%G.Scb>bGbJn[nancnqo+o0oWokvuw/(B")
(qdv "nz"	"$A158:@!G7GUKxMbOGR{nGoPocvnvww,(B")
(qdv "oa"	"$A777o7~899IE9HyP|R\V+k{m+w;(B")
(qd "ob"	"$As?(B")
(qdv "oc"	"$A5q6GJ$LEPHS7T_T`kMkbksky(B")
(qdv "od"	"$A0r3&4M7>7N9:?h@_E8EtLZO#P2PXTQVcVzj`k?kTkqpPthw=wG(B")
(qdv "oe"	"$A=EAi['[-[1[:jf(B")
(qdv "of"	"$A8lB8DdEbIDIEL%LyM,V\XOaYj]k`kzk|w<w?w@wD(B")
(qdv "og"	"$AE'UKUMjb(B")
(qd "oh"	"$Akk(B")
(qdv "oi"	"$A7tD$JjR=kDk^km(B")
(qdv "oj"	"$A21222F8-8UI2I>TrVbXWjgk>kckr(B")
(qdv "ok"	"$AOXSDkEkZ(B")
(qdv "ol"	"$AI1Nuk[k_k}wA(B")
(qdv "om"	"$A8NEVF"SCkBk]l"wB(B")
(qd "on"	"$AYm(B")
(qdv "oo"	"$A0\2a7g8,8T9G;_=:DTEsGxMxPWR8X3XRkGkHkXkakfknku(B")
(qdv "or"	"$A3<;K=3?oFjO;VWkNkOw:(B")
(qdv "os"	"$A07>_LsQ|S$ktkwwCwF(B")
(qdv "ot"	"$A090{4`7J<!<{ErF%K&M.MQMsXP[Kj1j\k@kCkFkJkSkWkdkgl)l+l,l.wE(B")
(qdv "ou"	"$A1a6D@0TyV,W,kRkYkol!(B")
(qdv "ov"	"$A1lBvI^KhMHO%OY[JjdkKkekjklkx(B")
(qdv "oy"	"$A5$5(838X<z>^A3CsDeE_G;GRTBTtV|Xh_1efkUkVkpk~w>(B")
(qdv "oz"	"$A147K:!DZE7HbNTRCRHXQXSjakIkLkQk\khkil$(B")
(qdv "pa"	"$A6P;:CFW:cSg&g1g7gAgO(B")
(qd "pb"	"$Ag)(B")
(qdv "pc"	"$A2x9kN,g6(B")
(qdv "pd"	"$A435^7D7W;CC`DVHRN3PeQ$SWT<VUcGcUg(g+g0p8pB(B")
(qd "pe"	"$A0s(B")
(qdv "pf"	"$A8s8x=a@+BgIII\NJT5`NcLg*g8g:(B")
(qdv "pg"	"$AcOgYwO(B")
(qdv "ph"	"$AU@g>(B")
(qdv "pi"	"$APxcWgC(B")
(qdv "pj"	"$A1U8?fzf{g#g,g2gH(B")
(qdv "pk"	"$A;fcHg!g[(B")
(qdv "pl"	"$ALPOPgRgX(B")
(qdv "pm"	"$A0m2{4B<)NEOKRog-g<(B")
(qdv "po"	"$A1A1`3q8Y<6=I=J>nDINFcIc[g'g.g/g4gBgIgJ(B")
(qdv "pp"	"$ACEfy(B")
(qdv "pr"	"$A>@HrIpU"(B")
(qdv "ps"	"$ABFKgS'V/cVcYgGgM(B")
(qdv "pt"	"$A4?<M>x@BBZHFI4I~M3OgQKTDcJcNcRf|f}g"g5g9g?gQ(B")
(qdv "pu"	"$A<dCeKuO8PwQVcMcQcTg$g;g=gEgFgLgU(B")
(qdv "pv"	"$A7l@;A7BLW[cKgDgNgPgTgWti(B")
(qd "pw"	"$Af~(B")
(qd "px"	"$ACv(B")
(qdv "py"	"$A7':l<L<j>-E&H^K?O_V=WicEcFcPcXg%gKgVg\(B")
(qdv "pz"	"$A:R<(IAW]g3g@gSgZ(B")
(qdv "qa"	"$A<I?%F$FFSAT&fum*(B")
(qd "qb"	"$A4!(B")
(qdv "qc"	"$A9hB?fmmT(B")
(qdv "qd"	"$A0u3R8%9->">TBkBmDKFvHuJiSBV`W)[Bevfefpfsm8mBmImZpMpV(B")
(qdv "qe"	"$A?$L<(B")
(qdv "qf"	"$A>}BfT%UhVhffmQmUmf(B")
(qdv "qg"	"$ADkUEfxm^(B")
(qdv "qh"	"$AmVqb(B")
(qdv "qi"	"$A9oBrMTmL(B")
(qdv "qj"	"$A0-0~5<AKC,FoQ0ShTPWSXYf]f^fbfvm<mJmg(B")
(qdv "qk"	"$A4E:kORm@(B")
(qdv "ql"	"$A5z=8Haflm]m_ma(B")
(qdv "qm"	"$A1.5/AWEiH:KiQPRletfdfhfijqm2m7m:(B")
(qdv "qn"	"$ANyvJ(B")
(qdv "qo"	"$A255o8D:/<0EpF-G}H7HfJ;OuS2TRWNewfcfjm?mAmOm\mbp.qc(B")
(qdv "qq"	"$A@ZJ/qf(B")
(qdv "qr"	"$A=>IiK`Q1R}W$X>m1m=(B")
(qdv "qs"	"$A5b;GYcfDfkfw(B")
(qdv "qt"	"$A3Z3[5J7/<:?WArExI0JhKHMUMkQbRQR|Za[Oeuf_fgfnm6m;mHmMmW(B")
(qdv "qu"	"$A3P9B;!Nxfafrm>mPmXmYn&(B")
(qdv "qv"	"$A1C@yA%B5B<BbCVKofXftme(B")
(qd "qw"	"$A?s(B")
(qdv "qx"	"$A50G?I'rzs1(B")
(qdv "qy"	"$A2j3s4h5G<n<o=.?DCOCqEvQiS/X)X=erf`j.mEmFmGmKmRm[mcmd(B")
(qdv "qz"	"$A7Q:":'>1?3FDK6T$cZmDmS(B")
(qdv "ra"	"$A1/1Q2#;<I*VR`za#h%h&h(h.(B")
(qd "rb"	"$AQ~(B")
(qdv "rc"	"$A41<a@ma!a"h*h-(B")
(qdv "rd"	"$A273)=mAaBjJ&K'QlSkSqXg`xgbgctd(B")
(qdv "re"	"$ASJZ}Z~gpg|w](B")
(qdv "rf"	"$AL{WAghglgsh"h)h3h4wXwY(B")
(qdv "rg"	"$A3$@EEaUJw_(B")
(qd "rh"	"$Ah/(B")
(qdv "ri"	"$AQk^J`ya%gxh1(B")
(qdv "rj"	"$AXV`|g`gfgqgyh$(B")
(qd "rk"	"$A7)(B")
(qdv "rl"	"$AAUVih!h2w[(B")
(qdv "rm"	"$A121OE*KAXBa$gmguh0(B")
(qd "rn"	"$A<x(B")
(qdv "ro"	"$A139i:wA'C5H=HpI:IvJ7JUTdgdl3mimjmkwW(B")
(qdv "rq"	"$A1LGY(B")
(qdv "rr"	"$A0_0`4.6OIjMuVPX-[Ic]ge(B")
(qdv "rs"	"$A5d<=VDgngwh+h,w^(B")
(qdv "rt"	"$A111H1P5g7+9e;O@@ApECE}MfOVUdgagggrgzg{g~wTwUwVwZw\(B")
(qdv "ru"	"$A7y=T>IAYC1EAGzSI`~a&gjh#h'(B")
(qdv "rv"	"$A;7=tGmGrWWgvg}(B")
(qd "rx"	"$Arc(B")
(qdv "ry"	"$A0;<`>BJ@JzR5R7VQc^gigkw`(B")
(qdv "rz"	"$A7G9s>AKvOMRTV!`}goh6j[(B")
(qdv "sa"	"$A4H7^<KE+E-I)K!O1W\fBfHm&tH(B")
(qd "sb"	"$A2m(B")
(qdv "sc"	"$A6JHQK\M^PU[Pf2(B")
(qdv "sd"	"$A5\7A7V<tAgBhDLE,NLOhR/Yb`{eseyf"f#f3f7fLfOnCpApCpKpOte(B")
(qdv "se"	"$A5&DHV#[7tJ(B")
(qdv "sf"	"$A9C9H<^HgIFJ<J^fR(B")
(qdv "sg"	"$ADoIoYff;(B")
(qd "sh"	"$Af<(B")
(qdv "si"	"$A5l8~9X<5C@Q}SifFi`(B")
(qdv "sj"	"$A:CG0LjWpf%f9fCf[f\(B")
(qdv "sk"	"$A9+WHfV(B")
(qdv "sl"	"$A8a<eC=CCf-iC(B")
(qdv "sm"	"$A2"5%<iQre{f0f>f?fQ(B")
(qdv "sn"	"$A5y8*(B")
(qdv "so"	"$A5U888><'>jD[K7Pve}f)f/f4f:f@fE(B")
(qd "sp"	"$Af5(B")
(qd "sq"	"$Af!(B")
(qdv "sr"	"$A8+=?ItQx(B")
(qdv "ss"	"$A0K=*E.fI(B")
(qdv "st"	"$A0V6R@QCdCnD]F?G<K}MqNMR&eze|e~f,f1f8fGj5(B")
(qdv "su"	"$A;i<fCDD7FUGuJWOSRvTxWEf(f=fMfWtI(B")
(qdv "sv"	"$APufNfP(B")
(qd "sx"	"$AfJ(B")
(qdv "sy"	"$A2n6p8G=c@<EhP_RfYef$f'f+fAfStFtGtK(B")
(qdv "sz"	"$A0dF6G8KLO[RLS{f&(B")
(qdv "ta"	"$A0.0c0f6N7"74:v;YJ\KRO"O$RsX;[Ek<l/m,p'pWt2(B")
(qdv "tb"	"$AT@a.t.(B")
(qdv "tc"	"$A49D5HIIzI|VXt5t>(B")
(qdv "td"	"$A0?1*1R2/4R5:5D5i6l7-9*94>K>dBQDqERIWI{NZNoNpPYQ.TATHX?YhYiYjYk[>g]p1p6p7p;p@pGpNt*t3ww(B")
(qdv "te"	"$AC.GdGqS!Zy[![%[([,[.[/[6l`(B")
(qdv "tf"	"$A4,8f:Q:sI`MLjtjujvp)t?uuwy(B")
(qd "tg"	"$AtA(B")
(qd "th"	"$ARI(B")
(qdv "ti"	"$A0B3t6?J'X2^Ik9uxw{(B")
(qdv "tj"	"$A2r386g7u9N:u=K>tEYIdImJ#JVLXVFXXXffZjwj~k!t4(B")
(qdv "tk"	"$A6*C4OO(B")
(qdv "tl"	"$A2I326c6d8^:LQ,VlhIk:t+uw(B")
(qdv "tm"	"$A0]104G87E#EFG'I}JMK4PFR^X:^Ct(w}(B")
(qd "tn"	"$A9;(B")
(qdv "to"	"$A5PDAG{LIOrSmV[jzj{k8k;p(t/t9(B")
(qd "tq"	"$AEM(B")
(qdv "tr"	"$A1G4(9T;J=oF,GGKygth7t1(B")
(qdv "ts"	"$A1xMWS_jxwz(B")
(qdv "tt"	"$A0(0|2J2U656f6y729m:=<8="?}BRC+C2CYF9FGHiL:MRMnOHTmU1UWX/X0X1X4X7Ylajg_jrjsk$k%k&k*k+k=t0t:uywHwIwKwLwMwNw|(B")
(qdv "tu"	"$A0W204Q6\7&7,9O;h>J?4AtE@LpN~R(STV<W&WTX6g^mrp+p,t6t8t<t=uvwx(B")
(qdv "tv"	"$A1|3K6{@VH*LrM'O5jytm(B")
(qd "tw"	"$Aut(B")
(qdv "ty"	"$AELGpJOJsNRQ*X5nBsBt,t-t;w~(B")
(qdv "tz"	"$A3b??C3K3P@PkS1T^VJl&q}r#t't7(B")
(qdv "ua"	"$A6w?RB|E/K<O>S^Wn[GjSm|n$n2wq(B")
(qd "ub"	"$Awm(B")
(qdv "uc"	"$A?Q@oA?D+D@FhJ{K/M+PGjWmum}n!n*n6n>vBvD(B")
(qdv "ud"	"$AD8DPENJqM<N'Q<RW[C`q`rfUj@mmm{n?wn(B")
(qdv "ue"	"$A0:3r<4jAjD(B")
(qdv "uf"	"$A9L;XBTNnO9U0UQ`tjOjPmtn9n:wo(B")
(qdv "ug"	"$A3?3WN7Q[`wjYt^(B")
(qdv "uh"	"$A=^JGLbh8(B")
(qdv "ui"	"$AD,RrS3j;jRn%n0wr(B")
(qdv "uj"	"$A3k6"7#;{J1L^MER0Uv`nmln.(B")
(qdv "uk"	"$A0UQ#j<(B")
(qdv "ul"	"$A2G9{:Z@'CACPGFUUj=jVlcmyn<(B")
(qdv "um"	"$A:5EOK2LoTNTgUV`vj:jJjMmxn"n,(B")
(qdv "un"	"$AB^b7mwwp(B")
(qdv "uo"	"$A>&?tBwCwFTGgINN8VuX.`s`ujNjUkPmnn+n1n=(B")
(qd "uq"	"$AG-(B")
(qdv "ur"	"$A6&9z<W=gMzNzj?n/(B")
(qdv "us"	"$A`ojLjTn((B")
(qdv "ut"	"$A0M6Z;N<H@%CaE~H&HkKDLwMmNcO~S0T0Yn`pj9jGjKk,mompmqmvm~n3n5n;wu(B")
(qdv "uu"	"$A050<2}6C;^>'C0CiI9JnJoJpM9U#jBn@wv(B")
(qdv "uv"	"$A1)>0@[A@FXn)n7wt(B")
(qd "uw"	"$A?u(B")
(qdv "uy"	"$A5)5ICKC_D?HUOTT;VCj-jEjIjQjXmzn'n8t_(B")
(qdv "uz"	"$A9a?EGtP*T2WoWrr+(B")
(qdv "va"	"$A75eReZ(B")
(qd "vb"	"$AR#(B")
(qdv "vc"	"$A3>H8M"eSe_(B")
(qdv "vd"	"$A1_5]<#ASBuM8N%Q2e](B")
(qdv "ve"	"$AS-eQ(B")
(qdv "vf"	"$AG2JJKlLvTlVpW7eHeJeKeUeaed(B")
(qd "vg"	"$AMK(B")
(qdv "vi"	"$A4o5|<bKM(B")
(qdv "vj"	"$A9}AISXSbWqeB(B")
(qd "vk"	"$ATK(B")
(qdv "vl"	"$ACTJvM>UZ(B")
(qdv "vm"	"$A1E1\7j=(=xA,G(eCeLe`eb(B")
(qd "vn"	"$AB_(B")
(qdv "vo"	"$A1iM(Q{SvUbeDeMeNePeWe[e\(B")
(qdv "vr"	"$A3Q9d=|JEeX(B")
(qd "vs"	"$AeT(B")
(qdv "vt"	"$ADfFyIYLSQ!R]T6eFec(B")
(qdv "vu"	"$A1F3Y5@5O6]6tFHJ!TbeYe^(B")
(qdv "vv"	"$A4~;9KYP!Q7[HeAeGeOeVfY(B")
(qdv "vy"	"$A6:Q8QSeIee(B")
(qdv "vz"	"$AREeE(B")
(qdv "wa"	"$A6H7O8PF#J]OCR_T8b!bQpkp}q!q#q+q/q0(B")
(qdv "wb"	"$A0)p^(B")
(qdv "wc"	"$A@e@jB.L1PIQ9QcW/Wyayb5b[b\pnpyq*q3(B")
(qdv "wd"	"$A97@w@x@zJ(L[O/QqS%aoatpQp\p]p_plpv(B")
(qdv "we"	"$A5R@*@HL5Zw(B")
(qdv "wf"	"$A3U4=4q5jF&LFa~b&phpmpq(B")
(qdv "wg"	"$A3=:[:]@GT3b+wP(B")
(qd "wh"	"$Au>(B")
(qdv "wi"	"$A:o;><2GlOAQaS|pzq"(B")
(qdv "wj"	"$A2^3x8.:qA!AFD&D|HhL|PrUxVLXcawa{b"p[pbpxwj(B")
(qdv "wk"	"$Aavpgwa(B")
(qdv "wl"	"$A42BiCSJ|a|b#b.b4bSiMwewg(B")
(qdv "wm"	"$A4a?bQ"QwUNXGamb'b/b3bTbXb]pwq1wk(B")
(qdv "wn"	"$Ab$wi(B")
(qdv "wo"	"$A2!2B7h8/;+=FE1H3M4S8S9azb)b,bZb_q$(B")
(qd "wp"	"$Apo(B")
(qd "wq"	"$AD%(B")
(qdv "wr"	"$A1T=j?qH,XIXKarpfq-wf(B")
(qdv "ws"	"$Aptp|q%q(wh(B")
(qdv "wt"	"$A0L1S1q4/4C6r788m>GANB9D'ESSLUnalaqasb%b0bObPbRbVp`peppq,wJwbwd(B")
(qdv "wu"	"$A2~:|>NA.ATAvC(CmOaVmXHXLb-bUpaps(B")
(qdv "wv"	"$A?5M%T-axb(b2b^q&q'q)wc(B")
(qdv "ww"	"$A3'akan(B")
(qdv "wx"	"$A6@p~rW(B")
(qdv "wy"	"$A3I5W6;>7>Q>RCMF]J"JyM~NANlOLPgS&V"apaua}b*j0pcpip{(B")
(qdv "wz"	"$A178}9KCRL!XFXJXMb1bWbYpdpjprpuq.q2(B")
(qdv "xa"	"$Arsrts#s,s3s6s=t$(B")
(qdv "xc"	"$AM\rIrNrqs+sO(B")
(qdv "xd"	"$A5Z=nBlI8rCrHrJrOrfs&sFsJsKsQsSs^(B")
(qdv "xe"	"$ArksL(B")
(qdv "xf"	"$A4p8r9\V)W-rArRs%s-s5s7s8sTsUsWsXsh(B")
(qdv "xi"	"$A4X?jP&r6rLrZs!ss(B")
(qdv "xj"	"$A2>3o5H7{<}QArBr[r]rurvsDs](B")
(qdv "xk"	"$A4[r<(B")
(qdv "xl"	"$A1?5{Vkr\rdrnr{sv(B")
(qdv "xm"	"$A0v2u7d8MKcr=rUrVrgrrr~s/s0sGsYsjslt!(B")
(qd "xn"	"$ABa(B")
(qdv "xo"	"$A8]9?:{?p@iF*HdM2NCNOROS<r8rErTrYr_rhroryr}s$sEsMs`sbscsdsfstsu(B")
(qd "xp"	"$A<r(B")
(qdv "xr"	"$A;HV{r;r>rSras\sksosrs}(B")
(qdv "xs"	"$A;I;~B(Cxr`rws((B")
(qdv "xt"	"$A0J1JA}I_KqMIS,V~r0r1r4r7r9r:rMr^rirjs2sHsZsasnswsxs{s|(B")
(qdv "xu"	"$A2-5Q;W<.@/OdP+r@rDrPrermrprxs)s4s9sIsgsit"t&(B")
(qdv "xv"	"$A2_B]EqQQWkrQs"s*sVs[(B")
(qd "xx"	"$A3f(B")
(qdv "xy"	"$A6j7$9F:g<c@:G)Gyr5rGrbrls:sCsNsRs_smspszs~t#(B")
(qdv "xz"	"$AO:rFsPsesqsyt%(B")
(qdv "ya"	"$A4O6q82:);s?VDtH!KaO<ODibl}m$ty(B")
(qdv "yc"	"$A;tPMPQQEVA[Qv/v2(B")
(qdv "yd"	"$A6/7S9&?wAcF8GIMrNmQ3Q;QIWCX$[=idp0p9p?pDqvtgv'v(v,(B")
(qdv "ye"	"$A9"ADCpP0POR.ZuZvZ{Z|[$[*[2[3[8[9(B")
(qdv "yf"	"$A4y7q>[?a@RB6Naqxt~u$u,u1u6(B")
(qdv "yg"	"$AAQDpUpwQ(B")
(qdv "yh"	"$Au.u;u<(B")
(qdv "yi"	"$AA*L*LlQju3(B")
(qdv "yj"	"$A4<4g5=6!818g;.=M?/?IAPDMPLQ@SZX!X+Xbihqtqwtzt|u*(B")
(qdv "yk"	"$ATFTM(B")
(qdv "yl"	"$A@uARAXJbKVKZUiu)u4u8(B")
(qdv "ym"	"$A6z8I<_?*EyF=Wmiiq{t{(B")
(qdv "yn"	"$A4uvF(B")
(qdv "yo"	"$A0T1B1{6x8R8|9%@vA=F.M*OvPhPoQ)SjTYU~VBX*quu'u-u/v)v1(B")
(qdv "yr"	"$A3j;-H)X"XAu(v+(B")
(qdv "ys"	"$AH"K#R*V0ikv.(B")
(qdv "yt"	"$A1"5"9.;jD^EdGLK@M_McN^PNR"RRT*UgX#iejjr3t}u"u#u+u2v=(B")
(qdv "yu"	"$A0Y4W:(=z@WC8C9CfF0K*NwSOu%v0(B")
(qdv "yv"	"$A2;9/CQF1J>cCv*(B")
(qdv "yw"	"$AQOu&(B")
(qdv "yx"	"$A2OHZr2rK(B")
(qdv "yy"	"$A2P3\696~7!7I8j9$;%;r=dHVH}J-J=MaNWNdNeQGR;S[U5U}V3X'X(X,XN_._/_0icigijilj'j)j/txu0u5u7(B")
(qdv "yz"	"$A6%8h91<VMgOBOnR3ifj>qyq|r(r,u!v-(B")
(qdv "za"	"$A84BGDnFgJeKKPpSzTuj7l~(B")
(qd "zb"	"$As>(B")
(qdv "zc"	"$A4F9^B,Wx[Vosotv;(B")
(qdv "zd"	"$A8kAdAnB2p5tatb(B")
(qdv "ze"	"$AAZC|P6[&(B")
(qdv "zf"	"$A:,:OIaU<V*v$v6v9(B")
(qdv "zg"	"$A2MJ3t@v8(B")
(qd "zh"	"$Av:(B")
(qdv "zi"	"$AH1J8S](B")
(qdv "zj"	"$A449t=#DCJfSaVqsA(B")
(qd "zk"	"$A;a(B")
(qdv "zl"	"$A2q5cS`W@Yai0tS(B")
(qdv "zm"	"$ADjI!KJNgNhP1W?r-(B")
(qdv "zo"	"$A3]<9>X?OA2CtE0G]H#Y_k6r/u~(B")
(qdv "zq"	"$A=qmN(B")
(qdv "zr"	"$A8v=C=iH+or(B")
(qd "zs"	"$A0+(B")
(qdv "zt"	"$A1k2=2V4K;"BXFrX9X@j2jhjljok(mhr.v3v5v7(B")
(qdv "zu"	"$A6=B1C?VGXUn#n4(B")
(qdv "zv"	"$A71WOY[Y\Y`(B")
(qdv "zy"	"$A014T5*6L7U7z8W:$:PBHDJFsFxGbGhIOP)PiQuU=V9YZY]Y^j(k-k.k/k0k1k2k3k4k5u:v4w*(B")
(qdv "zz"	"$A4S?(AlB-F5GjHKL0U'UjVZj_l#l(r"r%(B")

(quail-setup-current-package)
