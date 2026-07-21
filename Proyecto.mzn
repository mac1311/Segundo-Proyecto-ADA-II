% ============================================================
% Proyecto.mzn
% Modelo genérico para el Problema de Minimizar la Polarización (MinPol)
% ============================================================

% ---------- Parámetros de entrada ----------
int: n;                          % número total de personas
int: m;                          % número de opiniones posibles
set of int: I = 1..m;

array[I] of int: p;              % distribución inicial de personas por opinión
array[I] of float: v;            % valor de cada opinión (asumido no-decreciente en i)
array[I] of float: ce;           % costo extra por llegar a una opinión inicialmente vacía
array[I,I] of float: c;          % c[i,j] = costo unitario de mover una persona de i a j
float: ct;                       % costo total máximo permitido
int: maxM;                       % número máximo de movimientos permitido

constraint assert(sum(i in I)(p[i]) = n, "sum(p) debe ser igual a n");

% ---------- Constantes derivadas ----------
array[I] of int: e = [ if p[j] = 0 then 1 else 0 endif | j in I ];

% K[i,j]: costo unitario efectivo de mover UNA persona de i a j (constante, no depende de x)
array[I,I] of float: K = array2d(I, I,
   [ if i = j then 0.0
     else c[i,j] * (1.0 + int2float(p[i]) / int2float(n)) + int2float(e[j]) * ce[j]
     endif
   | i in I, j in I ]);

int: pos1 = (n + 1) div 2;        % posición del primer estadístico de orden para la mediana
int: pos2 = (n + 2) div 2;        % (pos1 = pos2 si n es impar)

% D[i,k1,k2] = |v[i] - (v[k1]+v[k2])/2|  (distancia precalculada, constante)
array[I,I,I] of float: D = array3d(I, I, I,
   [ abs(v[i] - (v[k1] + v[k2]) / 2.0) | i in I, k1 in I, k2 in I ]);

% ---------- Variables de decisión ----------
array[I,I] of var 0..n: x;               % x[i,j]: personas movidas de i a j
constraint forall(i in I)(x[i,i] = 0);
constraint forall(i in I)( sum(j in I where j != i)(x[i,j]) <= p[i] );

% ---------- Distribución final ----------
array[I] of var 0..n: q;
constraint forall(i in I)(
   q[i] = p[i] - sum(j in I where j != i)(x[i,j])
              + sum(k in I where k != i)(x[k,i])
);

% ---------- Suma acumulada (para localizar la mediana) ----------
array[0..m] of var 0..n: S;
constraint S[0] = 0;
constraint forall(i in I)( S[i] = S[i-1] + q[i] );

% ---------- Localización de la mediana: g1, g2 (codificados one-hot) ----------
array[I] of var 0..1: y1;   % y1[k]=1  <=>  la posición pos1 cae en el grupo k
array[I] of var 0..1: y2;   % y2[k]=1  <=>  la posición pos2 cae en el grupo k

constraint sum(k in I)(y1[k]) = 1;
constraint sum(k in I)(y2[k]) = 1;

constraint forall(k in I)( y1[k] = 1 -> (S[k-1] < pos1 /\ pos1 <= S[k]) );
constraint forall(k in I)( y2[k] = 1 -> (S[k-1] < pos2 /\ pos2 <= S[k]) );

% ---------- Linealización z[k1,k2] = y1[k1] * y2[k2] ----------
array[I,I] of var 0..1: z;
constraint forall(k1 in I, k2 in I)( z[k1,k2] <= y1[k1] );
constraint forall(k1 in I, k2 in I)( z[k1,k2] <= y2[k2] );
constraint forall(k1 in I, k2 in I)( z[k1,k2] >= y1[k1] + y2[k2] - 1 );

% ---------- Linealización u[i,k1,k2] = q[i] * z[k1,k2] ----------
array[I,I,I] of var 0..n: u;
constraint forall(i in I, k1 in I, k2 in I)( u[i,k1,k2] <= n * z[k1,k2] );
constraint forall(i in I, k1 in I, k2 in I)( u[i,k1,k2] <= q[i] );
constraint forall(i in I, k1 in I, k2 in I)( u[i,k1,k2] >= q[i] - n * (1 - z[k1,k2]) );

% ---------- Restricciones de presupuesto ----------
constraint sum(i in I, j in I where j != i)( K[i,j] * x[i,j] ) <= ct;
constraint sum(i in I, j in I where j != i)( abs(i - j) * x[i,j] ) <= maxM;

% ---------- Función objetivo: polarización final ----------
var float: Pol = sum(i in I, k1 in I, k2 in I)( D[i,k1,k2] * u[i,k1,k2] );

solve minimize Pol;

output [
  "Movimientos (i -> j : cantidad):\n"
] ++
[ "  \(i) -> \(j) : \(x[i,j])\n" | i in I, j in I where fix(x[i,j]) > 0 ] ++
[
  "Distribucion final q = \(q)\n",
  "Polarizacion optima = \(Pol)\n"
];