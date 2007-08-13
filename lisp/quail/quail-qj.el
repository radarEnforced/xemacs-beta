(require 'quail)
;; # !Id: QJ.tit,v 1.1 1991/10/27 06:21:16 ygz Exp !
;; # HANZI input table for cxterm
;; # To be used by cxterm, convert me to .cit format first
;; # .cit version 1
;; ENCODE:	GB
;; MULTICHOICE:	NO
;; PROMPT:	$A::WVJdHk!KH+=G!K# (B
;; #
;; COMMENT Copyright 1991 by Yongguang Zhang.      (ygz@cs.purdue.edu)
;; COMMENT Permission to use/modify/copy for any purpose is hereby granted.
;; COMMENT Absolutely no warranties.
;; # define keys
;; VALIDINPUTKEY:	\040!"\043$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMN
;; VALIDINPUTKEY:	OPQRSTUVWXYZ[\134]^_`abcdefghijklmnopqrstuvwxyz{|}~
;; # the following line must not be removed
;; BEGINDICTIONARY
(quail-define-package "qj" "$AH+=G(B"
 nil
 "$A::WVJdHk!KH+=G!K# (B
 Copyright 1991 by Yongguang Zhang.      (ygz@cs.purdue.edu)
 Permission to use/modify/copy for any purpose is hereby granted.
 Absolutely no warranties."
 '(
  ("." . quail-next-candidate-block)
  (">" . quail-next-candidate-block)
  ("," . quail-prev-candidate-block)
  ("<" . quail-prev-candidate-block)
  (" " . quail-select-current)
  )
 nil t)

;; #
(qd "\040"	"$A!!(B")
(qd "!"	"$A#!(B")
(qd "\""	"$A#"(B")
(qd "\043"	"$A##(B")
(qd "$"	"$A#$(B")
(qd "%"	"$A#%(B")
(qd "&"	"$A#&(B")
(qd "'"	"$A#'(B")
(qd "("	"$A#((B")
(qd ")"	"$A#)(B")
(qd "*"	"$A#*(B")
(qd "+"	"$A#+(B")
(qd ","	"$A#,(B")
(qd "-"	"$A#-(B")
(qd "."	"$A#.(B")
(qd "/"	"$A#/(B")
(qd "0"	"$A#0(B")
(qd "1"	"$A#1(B")
(qd "2"	"$A#2(B")
(qd "3"	"$A#3(B")
(qd "4"	"$A#4(B")
(qd "5"	"$A#5(B")
(qd "6"	"$A#6(B")
(qd "7"	"$A#7(B")
(qd "8"	"$A#8(B")
(qd "9"	"$A#9(B")
(qd ":"	"$A#:(B")
(qd ";"	"$A#;(B")
(qd "<"	"$A#<(B")
(qd "="	"$A#=(B")
(qd ">"	"$A#>(B")
(qd "?"	"$A#?(B")
(qd "@"	"$A#@(B")
(qd "A"	"$A#A(B")
(qd "B"	"$A#B(B")
(qd "C"	"$A#C(B")
(qd "D"	"$A#D(B")
(qd "E"	"$A#E(B")
(qd "F"	"$A#F(B")
(qd "G"	"$A#G(B")
(qd "H"	"$A#H(B")
(qd "I"	"$A#I(B")
(qd "J"	"$A#J(B")
(qd "K"	"$A#K(B")
(qd "L"	"$A#L(B")
(qd "M"	"$A#M(B")
(qd "N"	"$A#N(B")
(qd "O"	"$A#O(B")
(qd "P"	"$A#P(B")
(qd "Q"	"$A#Q(B")
(qd "R"	"$A#R(B")
(qd "S"	"$A#S(B")
(qd "T"	"$A#T(B")
(qd "U"	"$A#U(B")
(qd "V"	"$A#V(B")
(qd "W"	"$A#W(B")
(qd "X"	"$A#X(B")
(qd "Y"	"$A#Y(B")
(qd "Z"	"$A#Z(B")
(qd "["	"$A#[(B")
(qd "\134"	"$A#\(B")
(qd "]"	"$A#](B")
(qd "^"	"$A#^(B")
(qd "_"	"$A#_(B")
(qd "`"	"$A#`(B")
(qd "a"	"$A#a(B")
(qd "b"	"$A#b(B")
(qd "c"	"$A#c(B")
(qd "d"	"$A#d(B")
(qd "e"	"$A#e(B")
(qd "f"	"$A#f(B")
(qd "g"	"$A#g(B")
(qd "h"	"$A#h(B")
(qd "i"	"$A#i(B")
(qd "j"	"$A#j(B")
(qd "k"	"$A#k(B")
(qd "l"	"$A#l(B")
(qd "m"	"$A#m(B")
(qd "n"	"$A#n(B")
(qd "o"	"$A#o(B")
(qd "p"	"$A#p(B")
(qd "q"	"$A#q(B")
(qd "r"	"$A#r(B")
(qd "s"	"$A#s(B")
(qd "t"	"$A#t(B")
(qd "u"	"$A#u(B")
(qd "v"	"$A#v(B")
(qd "w"	"$A#w(B")
(qd "x"	"$A#x(B")
(qd "y"	"$A#y(B")
(qd "z"	"$A#z(B")
(qd "{"	"$A#{(B")
(qd "|"	"$A#|(B")
(qd "}"	"$A#}(B")
(qd "~"	"$A#~(B")

(quail-setup-current-package)