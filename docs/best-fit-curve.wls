#!/usr/bin/env wolframscript
(* Modeling Search Complexity: R^(R-2) vs Predictor Complexity: R^4 *)

searchData = {
  {6, 0.058},
  {7, 3.353},
  {8, 304.064},
  {9, 4320.0},
  {10, 4140.057} (* Actual R=10 high-opt time *)
};

predictorData = {
  {6, 0.0001},
  {8, 0.0002},
  {10, 0.0003},
  {16, 0.0005},
  {32, 0.002}
};

(* Fit search complexity to a * R^(R-2) *)
searchModel = NonlinearModelFit[searchData, a * r^(r - 2), {a}, r];
searchCoeff = a /. searchModel["BestFitParameters"];

(* Fit predictor complexity to b * R^4 *)
predictorModel = NonlinearModelFit[predictorData, b * r^4, {b}, r];
predictorCoeff = b /. predictorModel["BestFitParameters"];

Print["--- Complexity Model Results ---"];
Print["Search Model (Wall Time): T(R) = ", searchCoeff, " * R^(R-2)"];
Print["Predictor Model (Wall Time): T(R) = ", predictorCoeff, " * R^4"];

Print["\n--- Extrapolated Estimates for R=10 ---"];
r10Search = searchCoeff * 10^(10 - 2);
r10Predict = predictorCoeff * 10^4;

Print["Exhaustive Search R=10: ", r10Search / 3600.0, " hours"];
Print["Predictor R=10: ", r10Predict * 1000.0, " milliseconds"];

Print["\n--- The Divergence Point ---"];
Print["R=10 Ratio: Search is ", r10Search / r10Predict, "x slower."];
Print["R=16 Search Estimate: ", (searchCoeff * 16^14) / (3600.0 * 24 * 365 * 10^6), " million years"];
Print["R=16 Predictor Estimate: ", predictorCoeff * 16^4, " seconds"];

(* Plotting *)
plot = LogPlot[{searchModel[r], predictorModel[r]}, {r, 2, 12},
  PlotStyle -> {Directive[Thick, Red], Directive[Thick, Blue]},
  PlotLegends -> {"Exhaustive Search (R^(R-2))", "Hamming Predictor (R^4)"},
  Frame -> True,
  FrameLabel -> {"Subgraph Size (R)", "Wall Time (Seconds)"},
  PlotLabel -> "Search vs. Predictor Complexity (Log Scale)",
  GridLines -> Automatic,
  Filling -> {1 -> {2}}
];

Export["docs/complexity-curves.png", plot];
Print["\n[✓] Complexity graph exported to docs/complexity-curves.png"];
