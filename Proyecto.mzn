% ============================================================
% Proyecto.mzn
% Modelo genérico para el Problema de Minimizar la Polarización (MinPol)
% ============================================================

% ---------- Parámetros de entrada ----------
int: n;                          % número total de personas
int: m;                          % número de opiniones posibles
array[1..m] of int: p;      % array de personas por opinion
array[1..m] of float: v;    % array de valores de opinión
array[1..m, 1..m] of float: c;      % Matriz de costos de mover de opinión i a opinión j 
array[1..m] of float: ce;           % Array de costo extra al mover a una opinión vacia 
float: ct;                         % Costo máximo permitido
int: maxM;                         % Movimientos máximos permitidos