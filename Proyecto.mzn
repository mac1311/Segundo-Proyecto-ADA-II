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
var float: polarizacion;


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


% --- CÁLCULO DE LA MEDIANA Y POLARIZACIÓN ---

% La mediana es la postura donde la población acumulada desde la izquierda y desde la derecha 
% cubre al menos la mitad de la población total ((n + 1) div 2)
constraint sum(i in 1..idx_mediana)(p_fin[i]) >= ((n + 1) div 2);
constraint sum(i in idx_mediana..m)(p_fin[i]) >= ((n + 1) div 2);

% Cálculo de la polarización final según la fórmula del problema
constraint polarizacion = sum(i in 1..m) (
    int2float(p_fin[i]) * abs(v[i] - v[idx_mediana])
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

