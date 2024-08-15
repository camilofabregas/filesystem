#!/bin/bash

cd prueba

echo "[TEST 1] Crear directorio 'carpeta1'"
mkdir carpeta1
echo "- Al hacer ls, solo deberia aparecer 'carpeta1':"
ls

echo ""

echo "[TEST 2] Cambiar directorio a 'carpeta1'"
echo "- El directorio actual deberia ser '.../fisopfs/prueba'"
pwd
echo "- Se cambia el directorio actual a 'carpeta1'"
cd carpeta1
echo "- El directorio actual deberia ser '.../fisopfs/prueba/carpeta1'"
pwd
echo "- Al hacer ls, no deberia aparecer nada:"
ls

echo ""

echo "[TEST 3] Crear archivo 'test.txt'"
touch test.txt
echo "- Al hacer ls, deberia aparecer el nuevo archivo 'test.txt':"
ls

echo ""

echo "[TEST 4] Escritura y lectura archivo 'test.txt'"
echo "- Se escribe 'Hola mundo' en el archivo"
echo "Hola mundo" > test.txt
echo "- Al hacer 'cat test.txt', deberia aparecer el contenido escrito en el archivo:"
cat test.txt
echo ""

echo ""

echo "[TEST 5] Borrar archivo 'test.txt'"
echo "- Al hacer ls, solo deberia aparecer 'test.txt':"
ls
echo "- Se borra el archivo con 'rm test.txt'"
rm test.txt
echo "- Al hacer ls, no deberia aparecer nada:"
ls

echo ""

echo "[TEST 6] Cambiar directorio con pseudodirectorio '..'"
echo "- Al hacer pwd, el directorio actual deberia ser '.../fisopfs/prueba/carpeta1'"
pwd
echo "- Se cambia el directorio con 'cd ..'"
cd ..
echo "- Al hacer pwd, el directorio actual deberia ser '.../fisopfs/prueba'"
pwd

echo ""

echo "[TEST 7] Crear dos directorios mas"
echo "- Se crea el directorio 'carpeta2' con 'mkdir carpeta2'"
mkdir carpeta2
echo "- Se crea el directorio 'carpeta3' con 'mkdir carpeta3'"
mkdir carpeta3
echo "- Al hacer ls, deberian aparecer 'carpeta1', 'carpeta2' y 'carpeta3':"
ls

echo ""

echo "[TEST 8] Stats de un archivo"
echo "- Se ingresa al directorio 'carpeta2'"
cd carpeta2
echo "- Se crea un archivo 'test.txt'"
touch test.txt
echo "- Se escriben las stats del archivo 'test.txt' al archivo 'stats_test.txt'"
stat test.txt > stats_test.txt
echo "- Al hacer 'cat stats_test.txt', se deberian imprimir las stats de 'test.txt'"
cat stats_test.txt
echo ""

echo ""

echo "[TEST 9] Directorios anidados"
echo "- Vuelvo al directorio 'prueba' con 'cd ..':"
cd ..
pwd
echo "- Entro al directorio 'carpeta 3':"
cd carpeta3
pwd
echo "- Creo un archivo de texto 'test.txt' con 'Hola mundo'"
echo "Hola mundo" > test.txt
echo "- Creo un directorio anidado 'carpeta_nivel2'"
mkdir carpeta_nivel2
echo "- Al hacer ls, deberian aparecer test.txt y carpeta_nivel2:"
ls
echo "- Entro al directorio 'carpeta_nivel2:'"
cd carpeta_nivel2
pwd
echo "- Creo un archivo de texto 'test.txt' con 'Hola mundo'"
echo "Hola mundo" > test.txt
echo "- Creo un directorio anidado 'carpeta_nivel3'"
mkdir carpeta_nivel3
echo "- Al hacer ls, deberian aparecer test.txt y carpeta_nivel3:"
ls
echo "- Entro al directorio 'carpeta_nivel3:'"
cd carpeta_nivel3
pwd
echo "- Creo un archivo de texto 'test.txt' con 'Hola mundo'"
echo "Hola mundo" > test.txt
echo "- Creo un directorio anidado 'carpeta_nivel4'"
mkdir carpeta_nivel4
echo "- Al hacer ls, deberian aparecer test.txt y carpeta_nivel4:"
ls
echo "- Entro al directorio 'carpeta_nivel4:'"
cd carpeta_nivel4
pwd
echo "- Creo un directorio anidado 'carpeta_nivel5', que deberia dar error al alcanzarse el maximo de directorios anidados soportados"
mkdir carpeta_nivel5
echo "- Al hacer ls, no deberia aparecer nada:"
ls

echo ""

echo "[TEST 10] ls recursivo en directorios anidados"
echo "- Vuelvo al directorio prueba:"
cd ..
cd ..
cd ..
cd ..
pwd
echo "- Al ejecutar 'ls -R', se deberian mostrar todos los directorios y archivos creados en los tests anteriores:"
ls -R
echo ""

echo "[TEST 11] Modificar archivo existente"
echo "- Vuelvo a carpeta2 para modificar el archivo 'test.txt':"
cd carpeta2
pwd
echo "- Modifico el archivo ingresando un nuevo texto"
echo "Chau mundo" > test.txt
echo "- Al hacer 'cat test.txt', deberia mostrarse el nuevo contenido:"
cat test.txt
echo ""

echo ""

echo "[TEST 12] Llenar filesystem"
echo "- Se crean nuevos archivos hasta llegar a la capacidad maxima del filesystem, para testear los limites"
echo "- Creo el archivo numero 13"
touch archivo13.txt
echo "- Creo el archivo numero 14"
touch archivo14.txt
echo "- Creo el archivo numero 15"
touch archivo15.txt
echo "- Creo el archivo numero 16"
touch archivo16.txt
echo "- Creo el archivo numero 17"
touch archivo17.txt
echo "- Creo el archivo numero 18"
touch archivo18.txt
echo "- Creo el archivo numero 19"
touch archivo19.txt
echo "- Creo el archivo numero 20"
touch archivo20.txt
echo "- Intento crear el archivo numero 21, pero deberia dar error"
touch archivo21.txt
echo "- Al ejecutar ls, deberian aparecer todos los archivos creados, menos el 21:"
ls

echo ""

echo "- Para probar la serializacion, desmontar y volver a montar el filesystem. Deberian aparecer todas los directorios y archivos creados anteriormente."

echo ""
