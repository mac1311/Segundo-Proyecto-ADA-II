RESUMEN DE INSTANCIAS GENERADAS POR EL GRUPO
=============================================

Las siguientes 5 instancias fueron diseñadas para probar diferentes aspectos
del modelo MinPol. Cada una tiene una característica distintiva que permite
evaluar el comportamiento del modelo bajo distintas condiciones.


INSTANCIA 1 (MinPol_G1.dzn) - CASO BASE SIMPLE
------------------------------------------------
n=8, m=3, p=[3,2,3]
v=[0.0, 0.5, 1.0]
ct=15.0, maxM=6

Caracteristica: Caso mas pequeno y simetrico. Las 3 opiniones estan
equidistantes (0.0, 0.5, 1.0). Distribucion simetrica.

Polarizacion inicial calculada:
  Mediana: opinion 2 (valor 0.5), posicion 4.5
  Pol = 3*|0.0-0.5| + 2*|0.5-0.5| + 3*|1.0-0.5|
  Pol = 3*0.5 + 2*0 + 3*0.5 = 1.5 + 0 + 1.5 = 3.0

Proposito: Verificar que el modelo funciona en el caso mas basico.
  Se espera que el solver encuentre rapido la solucion optima.


INSTANCIA 2 (MinPol_G2.dzn) - OPINIONES VACIAS INTERMEDIAS
------------------------------------------------------------
n=10, m=4, p=[4,0,0,6]
v=[0.0, 0.33, 0.67, 1.0]
ct=25.0, maxM=8

Caracteristica: Dos opiniones intermedias vacias. Se activa el costo
extra (ce) al mover personas a opiniones 2 o 3.

Polarizacion inicial calculada:
  Mediana: entre opinion 1 (pos4) y opinion 4 (pos5), valor = (0+1)/2 = 0.5
  Pol = 4*|0.0-0.5| + 0*|0.33-0.5| + 0*|0.67-0.5| + 6*|1.0-0.5|
  Pol = 4*0.5 + 0 + 0 + 6*0.5 = 2.0 + 3.0 = 5.0

Proposito: Evaluar si el modelo prefiere mover personas a opiniones
  ya existentes (mas barato) o abrir opiniones nuevas (costo extra).


INSTANCIA 3 (MinPol_G3.dzn) - ALTA POLARIZACION
-------------------------------------------------
n=20, m=5, p=[8,0,0,0,12]
v=[0.0, 0.25, 0.5, 0.75, 1.0]
ct=40.0, maxM=15

Caracteristica: Distribucion extremadamente polarizada - dos bandos
opuestos (opinion 1 y opinion 5), sin nadie en el medio.

Polarizacion inicial calculada:
  Mediana: opinion 5 (valor 1.0), posicion 10.5
  Pol = 8*|0.0-1.0| + 0 + 0 + 0 + 12*|1.0-1.0|
  Pol = 8*1.0 + 0 = 8.0

  NOTA: Con este calculo la mediana es 1.0 porque la opinion 5 tiene
  12 personas (posiciones 9 a 20), y las posiciones 10 y 11 caen ahi.
  Pero esto no es correcto. Re-calculamos:

  Personas ordenadas por valor:
    Pos 1-8:   valor 0.0  (opinion 1, 8 personas)
    Pos 9-20:  valor 1.0  (opinion 5, 12 personas)
  Mediana (pos 10 y 11): ambas en opinion 5, valor 1.0
  Pol = 8*|0.0-1.0| + 12*|1.0-1.0| = 8*1 + 12*0 = 8.0

Proposito: Medir que tan bien el modelo reduce polarizacion extrema.
  Con presupuesto de 40, se espera que pueda mover varias personas
  de los extremos hacia el centro para reducir la polarizacion.


INSTANCIA 4 (MinPol_G4.dzn) - PRESUPUESTO AJUSTADO
----------------------------------------------------
n=30, m=6, p=[10,5,0,5,0,10]
v=[0.0, 0.2, 0.4, 0.6, 0.8, 1.0]
ct=15.0, maxM=20

Caracteristica: Presupuesto muy restrictivo (ct=15) para 30 personas.
  El modelo debe ser muy selectivo con los movimientos.

Polarizacion inicial calculada:
  Personas ordenadas:
    Pos 1-10:  valor 0.0  (opinion 1)
    Pos 11-15: valor 0.2  (opinion 2)
    Pos 16-20: valor 0.6  (opinion 4)
    Pos 21-30: valor 1.0  (opinion 6)
  Mediana (pos 15 y 16): entre 0.2 y 0.6 = 0.4
  Pol = 10*|0.0-0.4| + 5*|0.2-0.4| + 0 + 5*|0.6-0.4| + 0 + 10*|1.0-0.4|
  Pol = 10*0.4 + 5*0.2 + 5*0.2 + 10*0.6
  Pol = 4.0 + 1.0 + 1.0 + 6.0 = 12.0

Proposito: Probar el compromiso costo-polarizacion cuando el
  presupuesto es muy limitado. Se esperan mejoras modestas.


INSTANCIA 5 (MinPol_G5.dzn) - GRAN ESCALA
-------------------------------------------
n=200, m=10, p=[30,25,0,20,0,15,0,40,35,35]
v=[0.0, 0.11, 0.22, 0.33, 0.44, 0.56, 0.67, 0.78, 0.89, 1.0]
ct=500.0, maxM=300

Caracteristica: Instancia de gran escala con 200 personas y 10 opiniones.
  Tres opiniones vacias intercaladas. Presupuesto generoso.

Polarizacion inicial calculada:
  Personas ordenadas:
    Pos 1-30:   valor 0.0   (opinion 1, 30 personas)
    Pos 31-55:  valor 0.11  (opinion 2, 25 personas)
    Pos 56-75:  valor 0.33  (opinion 4, 20 personas)
    Pos 76-90:  valor 0.56  (opinion 6, 15 personas)
    Pos 91-130: valor 0.78  (opinion 8, 40 personas)
    Pos 131-165:valor 0.89  (opinion 9, 35 personas)
    Pos 166-200:valor 1.0   (opinion 10, 35 personas)
  Mediana (pos 100 y 101): ambas en opinion 8, valor 0.78
  Pol = 30*|0.0-0.78| + 25*|0.11-0.78| + 20*|0.33-0.78|
        + 15*|0.56-0.78| + 40*|0.78-0.78| + 35*|0.89-0.78| + 35*|1.0-0.78|
  Pol = 30*0.78 + 25*0.67 + 20*0.45 + 15*0.22 + 40*0 + 35*0.11 + 35*0.22
  Pol = 23.4 + 16.75 + 9.0 + 3.3 + 0 + 3.85 + 7.7 = 64.0

Proposito: Probar escalabilidad. Con n=200 y m=10, el modelo tiene
  100 variables de movimiento (x[i][j]). Se espera que el solver
  maneje la instancia en un tiempo razonable.


TABLA RESUMEN
=============
Instancia | n   | m  | Pol.Inicial | ct    | maxM | Caracteristica
----------|-----|----|-------------|-------|------|------------------
G1        | 8   | 3  | 3.0         | 15.0  | 6    | Simple, simetrico
G2        | 10  | 4  | 5.0         | 25.0  | 8    | Opiniones vacias
G3        | 20  | 5  | 8.0         | 40.0  | 15   | Alta polarizacion
G4        | 30  | 6  | 12.0        | 15.0  | 20   | Presupuesto ajustado
G5        | 200 | 10 | 64.0        | 500.0 | 300  | Gran escala
