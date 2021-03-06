#pragma once

const int NagValueSize=165;
char* NagValues[NagValueSize] = {
     "",                                            // 0
     "!",
     "?",
     "!!",
     "??",
     "!?",
     "?!",
     "forced move (all others lose quickly)",
     "singular move (no reasonable alternatives)",
     "worst move",
     "drawish position",                             // 10
     "equal chances, quiet position",
     "equal chances, active position",
     "unclear position",
     "+/=",
     "=/+",
     "+/-",
     "-/+",
     "+-",
     "-+",
     "+-",                                           // 20
     "-+",
     "White is in zugzwang",
     "Black is in zugzwang",
     "White has a slight space advantage",
     "Black has a slight space advantage",
     "White has a moderate space advantage",
     "Black has a moderate space advantage",
     "White has a decisive space advantage",
     "Black has a decisive space advantage",
     "White has a slight time (development) advantage", // 30
     "Black has a slight time (development) advantage",
     "White has a moderate time (development) advantage",
     "Black has a moderate time (development) advantage",
     "White has a decisive time (development) advantage",
     "Black has a decisive time (development) advantage",
     "White has the initiative",
     "Black has the initiative",
     "White has a lasting initiative",
     "Black has a lasting initiative",
     "White has the attack",                                // 40
     "Black has the attack",
     "White has insufficient compensation for material deficit",
     "Black has insufficient compensation for material deficit",
     "White has sufficient compensation for material deficit",
     "Black has sufficient compensation for material deficit",
     "White has more than adequate compensation for material deficit",
     "Black has more than adequate compensation for material deficit",
     "White has a slight center control advantage",
     "Black has a slight center control advantage",
     "White has a moderate center control advantage",        // 50
     "Black has a moderate center control advantage",
     "White has a decisive center control advantage",
     "Black has a decisive center control advantage",
     "White has a slight kingside control advantage",
     "Black has a slight kingside control advantage",
     "White has a moderate kingside control advantage",
     "Black has a moderate kingside control advantage",
     "White has a decisive kingside control advantage",
     "Black has a decisive kingside control advantage",
     "White has a slight queenside control advantage",          // 60
     "Black has a slight queenside control advantage",
     "White has a moderate queenside control advantage",
     "Black has a moderate queenside control advantage",
     "White has a decisive queenside control advantage",
     "Black has a decisive queenside control advantage",
     "White has a vulnerable first rank",
     "Black has a vulnerable first rank",
     "White has a well protected first rank",
     "Black has a well protected first rank",
     "White has a poorly protected king",                          // 70
     "Black has a poorly protected king",
     "White has a well protected king",
     "Black has a well protected king",
     "White has a poorly placed king",
     "Black has a poorly placed king",
     "White has a well placed king",
     "Black has a well placed king",
     "White has a very weak pawn structure",
     "Black has a very weak pawn structure",
     "White has a moderately weak pawn structure",                  // 80
     "Black has a moderately weak pawn structure",
     "White has a moderately strong pawn structure",
     "Black has a moderately strong pawn structure",
     "White has a very strong pawn structure",
     "Black has a very strong pawn structure",
     "White has poor knight placement",
     "Black has poor knight placement",
     "White has good knight placement",
     "Black has good knight placement",
     "White has poor bishop placement",                                // 90
     "Black has poor bishop placement",
     "White has good bishop placement",
     "Black has good bishop placement",
     "White has poor rook placement",
     "Black has poor rook placement",
     "White has good rook placement",
     "Black has good rook placement",
     "White has poor queen placement",
     "Black has poor queen placement",
     "White has good queen placement",                                // 100
     "Black has good queen placement",
     "White has poor piece coordination",
     "Black has poor piece coordination",
     "White has good piece coordination",
     "Black has good piece coordination",
     "White has played the opening very poorly",
     "Black has played the opening very poorly",
     "White has played the opening poorly",
     "Black has played the opening poorly",
     "White has played the opening well",                                // 110
     "Black has played the opening well",
     "White has played the opening very well",
     "Black has played the opening very well",
     "White has played the middlegame very poorly",
     "Black has played the middlegame very poorly",
     "White has played the middlegame poorly",
     "Black has played the middlegame poorly",
     "White has played the middlegame well",
     "Black has played the middlegame well",
     "White has played the middlegame very well",                        // 120
     "Black has played the middlegame very well",
     "White has played the ending very poorly",
     "Black has played the ending very poorly",
     "White has played the ending poorly",
     "Black has played the ending poorly",
     "White has played the ending well",
     "Black has played the ending well",
     "White has played the ending very well",
     "Black has played the ending very well",
     "White has slight counterplay",                                     // 130
     "Black has slight counterplay",
     "White has moderate counterplay",
     "Black has moderate counterplay",
     "White has decisive counterplay",
     "Black has decisive counterplay",
     "White has moderate time control pressure",
     "Black has moderate time control pressure",
     "White has severe time control pressure",
     "Black has severe time control pressure",
// The NAG value below 139 is comming from Andrew Templeton on rgcc
     "with the idea",                                                      // 140
     "aimed against",
     "better is",
     "worse is",
     "equal is",
     "RR",
     "N",
     "weak point",
     "endgame",
     "line",
     "diagonal",                                                       // 150
     "White has pair of bishops",
     "Black has pair of bishops",
     "Bishops of opposite color",
     "Bishops of same color",
     "White has united pawns",
     "Black has united pawns",
     "White has separated pawns",
     "Black has separated pawns",
     "White has doubled pawns",
     "Black has doubled pawns",                                           // 160
     "White has passed pawn",
     "Black has passed pawn",
     "White has adv. in no. pawns",
     "Black has adv. in no. pawns"
};
