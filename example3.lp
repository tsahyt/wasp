hole(1..5).
pigeon(1..6).

#instance preplace/2.
{ preplace(P,H) : pigeon(P), hole(H) }.

1 { place(P,H) : hole(H) } 1 :- pigeon(P).
:- preplace(P,H), not place(P,H).
:- place(P1,H), place(P2,H), hole(H), P1 != P2.
