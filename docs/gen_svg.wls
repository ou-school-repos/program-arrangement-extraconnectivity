#!/usr/bin/env wolframscript

(* R=9 Graph *)
edges9 = {
  1 <-> 2, 1 <-> 3, 1 <-> 4, 1 <-> 5,
  2 <-> 6, 2 <-> 7,
  3 <-> 6, 3 <-> 8,
  4 <-> 7, 4 <-> 8,
  6 <-> 9, 7 <-> 9, 8 <-> 9
};

g9 = Graph[edges9,
  VertexLabels -> Automatic,
  GraphStyle -> "SmallNetwork",
  VertexSize -> Medium,
  ImageSize -> 400
];

Export["docs/R9_graph.svg", g9];
Print["Generated docs/R9_graph.svg"];

(* R=10 Graph *)
edges10 = {
  1 <-> 2, 1 <-> 3, 1 <-> 4, 1 <-> 5,
  2 <-> 6, 2 <-> 7, 2 <-> 8,
  3 <-> 6, 3 <-> 9,
  4 <-> 7, 4 <-> 9,
  5 <-> 8,
  6 <-> 10, 7 <-> 10, 9 <-> 10
};

g10 = Graph[edges10,
  VertexLabels -> Automatic,
  GraphStyle -> "SmallNetwork",
  VertexSize -> Medium,
  ImageSize -> 400
];

Export["docs/R10_graph.svg", g10];
Print["Generated docs/R10_graph.svg"];
