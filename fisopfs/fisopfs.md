# fisop-fs

## Diseño del File System

Para el diseño del File System se utilizo estructuras similares a las que
vimos en clase. Para representar un archivo creamos el struct `Inode`, esta estructura
se encarga de almacenar toda la metadata del archivo (ya sea un archivo o directorio).

```
// Struct that represents the Inode of the file system
// Used for saving metadata
typedef struct Inode {
	enum FileSystemType type;
	uid_t uid;
	gid_t gid;
	unsigned int size;
	time_t creation_time;
	time_t last_access_time;
	time_t last_modification_time;
	unsigned int assigned_block;
} Inode;
```

Los `Inode` se almacenan en una lista de `Inode` llamada `InodeTable`, que tiene
un máximo de elementos permitidos y define el máximo de archivos/directorios que puede
contener el File System. Para identificar que posiciones de la lista estan usados o no
implementamos una lista de booleanos del mismo tamaño, llamado `InodesBitmap`.

Para almacenar los datos del archivo/directorio utilizamos un struct `Block`, este bloque
almacena toda la información del archivo/directorio. El bloque contiene un union, utilizando el 
struct `FileBlock` en caso de ser el bloque de memoria de un archivo y el struct `DirBlock` en caso
de ser el bloque de memoria de un directorio.

Al igual que los `Inode`, los `Block` se almacenan en una lista llamada `DataRegion` y para
identificar que posiciones estan usadas se utiliza una lista de booleanos llamada `BlocksBitmap`.

```
// Struct that represents a block of memory
typedef struct Block {
	enum FileSystemType type;
	union {
		FileBlock file;
		DirBlock dir;
	};
} Block;
```

En el caso de que el elemento sea un archivo, el struct `FileBlock` contiene
los contenidos del archivo.

```
// Struct that represents a file memory block in the file system
typedef struct FileBlock {
	char content[MAX_FILE_CONTENT_SIZE];
} FileBlock;
```

En el caso de que el elemento sea un directorio, el struct `DirBlock` contiene
una lista de structs `DirectoryEntry`. Este struct se encarga de representar un elemento
dentro del directorio y almacena el nombre de ese elemento al igual que el número del inodo
con los metadatos del elemento.

```
// Struct that represents an entry (file or directory) inside a Directory
typedef struct DirectoryEntry {
	char filename[MAX_FILE_NAME_SIZE];
	unsigned int assigned_inode;
} DirectoryEntry;

// Struct that represents a directory memory block in the file system
typedef struct DirBlock {
	DirectoryEntry childs[MAX_NUMBERS_OF_INODES];
} DirBlock;
```

Para determinar que un `DirectoryEntry` esta vacio dentro de un `DirBlock` utilizamos el valor
`MOUNT_INODE_INDEX` que representa el número de `Inode` en el que esta la información del mount.

El File System también tiene definida su capacidad máxima mediante unas constantes:

- `#define MAX_NUMBER_OF_BLOCKS 20` define la cantidad máxima de bloques
- `#define MAX_NUMBERS_OF_INODES 20` define la cantidad máxima de inodos
- `#define MAX_FILE_CONTENT_SIZE 1000` define el tamaño máximo de archivo

En conjunto, establecen que la capacidad máxima del File System es de `20 * 1000 bytes = 20000 bytes`.
Es una capacidad pequeña para que sea más facil testear.
En caso de que se exceda el tamaño del File System, se arroja el error ENOSPC,
que indica que no hay espacio libre en el dispositivo.

## Directorios Anidados

El File System permite una cantidad máxima de directorios anidados, representado por la constante:
```
// Max number of nested directories allowed
#define MAX_NESTED_DIRECTORIES 4
```

El directorio anidado se almacena como cualquier otro archivo/directorio dentro del `DirectoryEntry` y cada una
de las operaciones de FUSE se encarga de separar el path del archivo e ir recorriendo cada Inodo hasta llegar
al archivo.


Ejemplo de Read:
```
	char paths[MAX_NESTED_DIRECTORIES][MAX_FILE_NAME_SIZE];
	
	// Obtiene cada nombre de archivo/directorio y la cantidad
	int count = separate_path(path, paths);
	
	// Obtiene el directorio padre del ultimo nombre
	// Ejemplo /home/Desktop/file.txt obtiene el Inodo de Desktop
	int parent_inode_index = get_nested_dir_inode(paths, count - 1);
	
	// Obtiene el DirectoryEntry del archivo
	DirectoryEntry *dir_entry =
	        find_directory_entry(parent_inode_index, paths[count - 1]);

	if (dir_entry == NULL) {
		return -ENOENT;
	}
```

## Serialización

Para la serialización, todos los structs mencionados anteriormente se almacenan
en el archivo `fs.fisopfs`. Al tener el número máximo de `Inode` y `Blocks` los datos
se serializan de la siguiente manera:

- Se serializa los datos del `BlocksBitmap`.
- Se serializa los datos del `InodeBitmap`.
- Se serializa los datos del `InodeTable`.
- Se serializa los datos del `DataRegion`.

Al ser arreglos, lo que se hace simplemente es ciclarlos utilizando su correspondiente constante 
(ej. `MAX_NUMBERS_OF_INODES` para `InodesBitmap`) y hacer fwrite de cada elemento al archivo.

Para cargar los datos serializados a la hora de montar el File System, al tener definidos
los tamaños de cada uno de los structs, se va leyendo el archivo en el mismo orden que se
serializo, utilizando la funcion fread.

## Pruebas

Para probar el filesystem, se implementó una serie de pruebas mediante scripts de bash.
Como precondición para poder ejecutar estas pruebas, es necesario correrlas desde un
filesystem "limpio", es decir, con un nuevo archivo `.fisopfs`.

- Salida de las pruebas:

```
[TEST 1] Crear directorio 'carpeta1'
- Al hacer ls, solo deberia aparecer 'carpeta1':
carpeta1

[TEST 2] Cambiar directorio a 'carpeta1'
- El directorio actual deberia ser '.../fisopfs/prueba'
/home/alejofabregas/Documents/sisop/tp4-filesystem/fisopfs/prueba
- Se cambia el directorio actual a 'carpeta1'
- El directorio actual deberia ser '.../fisopfs/prueba/carpeta1'
/home/alejofabregas/Documents/sisop/tp4-filesystem/fisopfs/prueba/carpeta1
- Al hacer ls, no deberia aparecer nada:

[TEST 3] Crear archivo 'test.txt'
- Al hacer ls, deberia aparecer el nuevo archivo 'test.txt':
test.txt

[TEST 4] Escritura y lectura archivo 'test.txt'
- Se escribe 'Hola mundo' en el archivo
- Al hacer 'cat test.txt', deberia aparecer el contenido escrito en el archivo:
Hola mundo

[TEST 5] Borrar archivo 'test.txt'
- Al hacer ls, solo deberia aparecer 'test.txt':
test.txt
- Se borra el archivo con 'rm test.txt'
- Al hacer ls, no deberia aparecer nada:

[TEST 6] Cambiar directorio con pseudodirectorio '..'
- Al hacer pwd, el directorio actual deberia ser '.../fisopfs/prueba/carpeta1'
/home/alejofabregas/Documents/sisop/tp4-filesystem/fisopfs/prueba/carpeta1
- Se cambia el directorio con 'cd ..'
- Al hacer pwd, el directorio actual deberia ser '.../fisopfs/prueba'
/home/alejofabregas/Documents/sisop/tp4-filesystem/fisopfs/prueba

[TEST 7] Crear dos directorios mas
- Se crea el directorio 'carpeta2' con 'mkdir carpeta2'
- Se crea el directorio 'carpeta3' con 'mkdir carpeta3'
- Al hacer ls, deberian aparecer 'carpeta1', 'carpeta2' y 'carpeta3':
carpeta1  carpeta2  carpeta3

[TEST 8] Stats de un archivo
- Se ingresa al directorio 'carpeta2'
- Se crea un archivo 'test.txt'
- Se escriben las stats del archivo 'test.txt' al archivo 'stats_test.txt'
- Al hacer 'cat stats_test.txt', se deberian imprimir las stats de 'test.txt'
  File: test.txt
  Size: 0         	Blocks: 0          IO Block: 4096   regular empty file
Device: 53h/83d	Inode: 6           Links: 1
Access: (0644/-rw-r--r--)  Uid: ( 1000/alejofabregas)   Gid: ( 1000/alejofabregas)
Access: 2023-07-02 23:13:52.000000000 -0300
Modify: 2023-07-02 23:13:52.000000000 -0300
Change: 2023-07-02 23:13:52.000000000 -0300
 Birth: -

[TEST 9] Directorios anidados
- Vuelvo al directorio 'prueba' con 'cd ..':
/home/alejofabregas/Documents/sisop/tp4-filesystem/fisopfs/prueba
- Entro al directorio 'carpeta 3':
/home/alejofabregas/Documents/sisop/tp4-filesystem/fisopfs/prueba/carpeta3
- Creo un archivo de texto 'test.txt' con 'Hola mundo'
- Creo un directorio anidado 'carpeta_nivel2'
- Al hacer ls, deberian aparecer test.txt y carpeta_nivel2:
carpeta_nivel2	test.txt
- Entro al directorio 'carpeta_nivel2:'
/home/alejofabregas/Documents/sisop/tp4-filesystem/fisopfs/prueba/carpeta3/carpeta_nivel2
- Creo un archivo de texto 'test.txt' con 'Hola mundo'
- Creo un directorio anidado 'carpeta_nivel3'
- Al hacer ls, deberian aparecer test.txt y carpeta_nivel3:
carpeta_nivel3	test.txt
- Entro al directorio 'carpeta_nivel3:'
/home/alejofabregas/Documents/sisop/tp4-filesystem/fisopfs/prueba/carpeta3/carpeta_nivel2/carpeta_nivel3
- Creo un archivo de texto 'test.txt' con 'Hola mundo'
- Creo un directorio anidado 'carpeta_nivel4'
- Al hacer ls, deberian aparecer test.txt y carpeta_nivel4:
carpeta_nivel4	test.txt
- Entro al directorio 'carpeta_nivel4:'
/home/alejofabregas/Documents/sisop/tp4-filesystem/fisopfs/prueba/carpeta3/carpeta_nivel2/carpeta_nivel3/carpeta_nivel4
- Creo un directorio anidado 'carpeta_nivel5', que deberia dar error al alcanzarse el maximo de directorios anidados soportados
mkdir: cannot create directory ‘carpeta_nivel5’: Operation not permitted
- Al hacer ls, no deberia aparecer nada:

[TEST 10] ls recursivo en directorios anidados
- Vuelvo al directorio prueba:
/home/alejofabregas/Documents/sisop/tp4-filesystem/fisopfs/prueba
- Al ejecutar 'ls -R', se deberian mostrar todos los directorios y archivos creados en los tests anteriores:
.:
carpeta1  carpeta2  carpeta3

./carpeta1:

./carpeta2:
stats_test.txt	test.txt

./carpeta3:
carpeta_nivel2	test.txt

./carpeta3/carpeta_nivel2:
carpeta_nivel3	test.txt

./carpeta3/carpeta_nivel2/carpeta_nivel3:
carpeta_nivel4	test.txt

./carpeta3/carpeta_nivel2/carpeta_nivel3/carpeta_nivel4:

[TEST 11] Modificar archivo existente
- Vuelvo a carpeta2 para modificar el archivo 'test.txt':
/home/alejofabregas/Documents/sisop/tp4-filesystem/fisopfs/prueba/carpeta2
- Modifico el archivo ingresando un nuevo texto
- Al hacer 'cat test.txt', deberia mostrarse el nuevo contenido:
Chau mundo

[TEST 12] Llenar filesystem
- Se crean nuevos archivos hasta llegar a la capacidad maxima del filesystem, para testear los limites
- Creo el archivo numero 13
- Creo el archivo numero 14
- Creo el archivo numero 15
- Creo el archivo numero 16
- Creo el archivo numero 17
- Creo el archivo numero 18
- Creo el archivo numero 19
- Creo el archivo numero 20
- Intento crear el archivo numero 21, pero deberia dar error
touch: cannot touch 'archivo21.txt': No space left on device
- Al ejecutar ls, deberian aparecer todos los archivos creados, menos el 21:
archivo13.txt  archivo15.txt  archivo17.txt  archivo19.txt  stats_test.txt
archivo14.txt  archivo16.txt  archivo18.txt  archivo20.txt  test.txt

- Para probar la serializacion, desmontar y volver a montar el filesystem. Deberian aparecer todas los directorios y archivos creados anteriormente.

```
