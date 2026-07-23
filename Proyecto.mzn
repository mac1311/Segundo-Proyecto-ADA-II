% ============================================================
% Proyecto.mzn
% Modelo genérico para el Problema de Minimizar la Polarización (MinPol)
% ============================================================

% ---------- Parámetros de entrada ----------
int: n;                             % Número total de personas
int: m;                             % Número de opiniones posibles
array[1..m] of int: p;              % Array de personas por opinión
array[1..m] of float: v;            % Array de valores de opinión
array[1..m, 1..m] of float: c;      % Matriz de costos de mover de opinión i a opinión j 
array[1..m] of float: ce;           % Array de costo extra al mover a una opinión vacía 
float: ct;                          % Costo máximo permitido
int: maxM;                          % Movimientos máximos permitidos

% Diferencias absolutas de opiniones precalculadas (constantes)
array[1..m, 1..m] of float: v_diff = array2d(1..m, 1..m, [
    abs(v[i] - v[j]) | i, j in 1..m
]);

% Cota Big-M para linealizar la polarización (usada también para acotar la variable)
float: M_pol = int2float(n) * max([abs(v[i] - v[j]) | i, j in 1..m]);

% --- VARIABLES DE DECISIÓN ---

% x[i, j]: Número de personas que pasan de la opinión i a la opinión j
array[1..m, 1..m] of var 0..n: x;

% Población final en cada opinión
array[1..m] of var 0..n: p_fin;

% Costo total acumulado de los movimientos
var 0.0..ct: costo_total;

% Índice de la opinión que resulta ser la mediana (1 a m)
var 1..m: idx_mediana;

% Valor de la polarización resultante (función objetivo)
var 0.0..M_pol: polarizacion;


% --- RESTRICCIONES BÁSICAS ---
constraint n > 0;
constraint m > 0;
constraint ct >= 0.0;
constraint maxM >= 0;
constraint sum(i in 1..m)(p[i]) = n;


% --- RESTRICCIONES Mas elaboradas ---
% 1. No se pueden mover de la opinión i más personas de las que había inicialmente
constraint forall(i in 1..m) (
    sum(j in 1..m where j != i)(x[i, j]) <= p[i]
);

% 2. Moverse a la misma opinión no tiene sentido (diagonal principal a cero)
constraint forall(i in 1..m) (
    x[i, i] = 0
);

% 3. Población final en cada opinión (p_fin)
constraint forall(j in 1..m) (
    p_fin[j] = p[j] - sum(k in 1..m)(x[j, k]) + sum(i in 1..m)(x[i, j])
);

% 4. Límite del número máximo de movimientos (maxM)
constraint sum(i in 1..m, j in 1..m where i != j) (
    abs(j - i) * x[i, j]
) <= maxM;

% 5. Cálculo del costo total
constraint costo_total = sum(i in 1..m, j in 1..m where i != j) (
    x[i, j] * (
        c[i, j] * (1.0 + int2float(p[i]) / int2float(n)) + 
        (if p[j] == 0 then ce[j] else 0.0 endif)
    )
);

constraint costo_total <= ct;


% --- CÁLCULO DE LA MEDIANA Y POLARIZACIÓN (Linealizado para MIP) ---

% Indicador: is_median[k] es true si k cumple la condición de mediana
array[1..m] of var bool: is_median;
constraint forall(k in 1..m) (
    is_median[k] <-> (
        sum(i in 1..k)(p_fin[i]) >= ((n + 1) div 2) /\
        sum(i in k..m)(p_fin[i]) >= ((n + 1) div 2)
    )
);

% Al menos una mediana válida debe existir
constraint sum(k in 1..m)(bool2int(is_median[k])) >= 1;

% idx_mediana debe ser una de las medianas válidas
constraint is_median[idx_mediana] = true;

% Cota Big-M para linealizar la polarización (definida arriba)

% Polarización >= valor real cuando is_median[k] es true (Big-M)
constraint forall(k in 1..m) (
    polarizacion >= sum(i in 1..m)(
        int2float(p_fin[i]) * v_diff[i, k]
    ) - (1.0 - int2float(bool2int(is_median[k]))) * M_pol
);


% --- RESOLUCIÓN Y SALIDA ---

solve minimize polarizacion;

output [
  "x = ", show(x), ";\n",
  "p_fin = ", show(p_fin), ";\n",
  "costo_total = ", show(costo_total), ";\n",
  "mediana = ", show(v[idx_mediana]), " (opinion ", show(idx_mediana), ");\n",
  "polarizacion = ", show(polarizacion), ";\n"
];

