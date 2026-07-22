% ============================================================
% Proyecto_opt.mzn
% Modelo optimizado con escalado entero (evita variables float y no linealidades)
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

% --- ESCALADO A ENTEROS (Constantes calculadas en tiempo de compilación) ---
int: SCALE = 10000;

array[1..m, 1..m] of int: c_scaled = array2d(1..m, 1..m, [
    round(SCALE * (c[i, j] * (1.0 + int2float(p[i]) / int2float(n)) + 
          (if p[j] == 0 then ce[j] else 0.0 endif)))
    | i, j in 1..m
]);

int: ct_scaled = round(SCALE * ct);

array[1..m] of int: v_scaled = [
    round(SCALE * v[i]) | i in 1..m
];

% Precalculamos las diferencias absolutas de opiniones en una matriz constante de enteros
array[1..m, 1..m] of int: v_diff = array2d(1..m, 1..m, [
    abs(v_scaled[i] - v_scaled[j])
    | i, j in 1..m
]);

% --- VARIABLES DE DECISIÓN ---

% x[i, j]: Número de personas que pasan de la opinión i a la opinión j
array[1..m, 1..m] of var 0..n: x;

% Población final en cada opinión
array[1..m] of var 0..n: p_fin;

% Costo total acumulado (escalado)
var 0..ct_scaled: costo_total_scaled;

% Índice de la opinión que resulta ser la mediana (1 a m)
var 1..m: idx_mediana;

% Polarización resultante (escalada)
var int: polarizacion_scaled;


% --- RESTRICCIONES BÁSICAS ---
constraint n > 0;
constraint m > 0;
constraint maxM >= 0;
constraint sum(i in 1..m)(p[i]) = n;


% --- RESTRICCIONES MÁS ELABORADAS ---
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

% 5. Cálculo del costo total escalado
constraint costo_total_scaled = sum(i in 1..m, j in 1..m where i != j) (
    x[i, j] * c_scaled[i, j]
);


% --- CÁLCULO DE LA MEDIANA Y POLARIZACIÓN (MIP-FRIENDLY BIG-M FORMULATION) ---

% is_median[k] es true si y solo si k cumple la condición de mediana
array[1..m] of var bool: is_median;
constraint forall(k in 1..m) (
    is_median[k] <-> (
        sum(i in 1..k)(p_fin[i]) >= ((n + 1) div 2) /\
        sum(i in k..m)(p_fin[i]) >= ((n + 1) div 2)
    )
);

% Al menos una mediana debe ser seleccionada
constraint sum(k in 1..m)(bool2int(is_median[k])) >= 1;

% Forzar que idx_mediana sea uno de los índices que son mediana
constraint is_median[idx_mediana] = true;

% Cota superior Big-M (n * máximo valor de opinión diferencia)
int: M = n * max([v_diff[i, j] | i, j in 1..m]);

% Restricción Big-M para la polarización
constraint forall(k in 1..m) (
    polarizacion_scaled >= sum(i in 1..m)(p_fin[i] * v_diff[i, k]) - (1 - bool2int(is_median[k])) * M
);


% --- RESOLUCIÓN Y SALIDA ---

solve minimize polarizacion_scaled;

output [
  "x = ", show(x), ";\n",
  "p_fin = ", show(p_fin), ";\n",
  "costo_total = ", show(int2float(costo_total_scaled) / int2float(SCALE)), ";\n",
  "mediana = ", show(v[idx_mediana]), " (opinion ", show(idx_mediana), ");\n",
  "polarizacion = ", show(int2float(polarizacion_scaled) / int2float(SCALE)), ";\n"
];
