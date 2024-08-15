# fisopfs

Repositorio para el esqueleto del [TP: filesystem](https://fisop.github.io/website/tps/filesystem) del curso Mendez-Fresia de **Sistemas Operativos (7508) - FIUBA**

> Sistema de archivos tipo FUSE.

## Respuestas teóricas

Utilizar el archivo `fisopfs.md` provisto en el repositorio

## Compilar

```bash
$ make
```

## Ejecutar

### Setup

Primero hay que crear un directorio de prueba:

```bash
$ mkdir prueba
```

### Iniciar el servidor FUSE

En el mismo directorio que se utilizó para compilar la solución, ejecutar:

```bash
$ ./fisopfs prueba/
```

Adicionalmente, se puede agregar un parametro opcional para ejecutar con una ruta custom para guardar el filesystem, por ejemplo:

```bash
$ ./fisopfs -f prueba mi_filesystem
```

### Verificar directorio

```bash
$ mount | grep fisopfs
```

### Utilizar el directorio de "pruebas"

En otra terminal, ejecutar:

```bash
$ cd prueba
$ ls -al
```

### Limpieza

```bash
$ sudo umount prueba
```

## Linter

```bash
$ make format
```

Para efectivamente subir los cambios producidos por el `format`, hay que hacer `git add .` y `git commit`.


## Pruebas

Se crearon un conjunto de pruebas que verifican el correcto funcionamiento del Fileystem. Las mismas se
encuentran en el archivo `tests.sh`. Para ejecutarlas, se debe utilizar una segunda terminal. 

En la primera se debe montar el Filesystem:
```bash
$ ./fisopfs -f prueba
```

En la segunda, se ejecutan las pruebas:
```bash
$ ./tests.sh
```
