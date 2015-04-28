/*
 * Grasa_Write.c
 *
 *  Created on: 23/11/2013
 *      Author: utnso
 */

#include "grasa.h"

/*
 *  @ DESC
 * 		Esta estructura creara carpetas en el filesystem.
 * 		Notese que no debe nodificarse el Bitmap, ya que todas las estructuras administrativas quedan marcadas.
 *
 * 	@ PARAM
 *
 * 		-path: El path del directorio a crear
 *
 * 		-mode: Contiene los permisos que debe tener el directorio y otra metadata
 *
 * 	@ RET
 * 		Seguramente 0 si esta ok, negativo si hay error.
 */
int grasa_mkdir (const char *path, mode_t mode){
			log_info(logger, "Mkdir: Path: %s", path);
	int nodo_padre, i, res = 0;
	struct grasa_file_t *node;
	char *nombre = malloc(strlen(path) + 1), *nom_to_free = nombre;
	char *dir_padre = malloc(strlen(path) + 1), *dir_to_free = dir_padre;

	if (determinar_nodo(path) != -1) return -EEXIST;

	split_path(path, &dir_padre, &nombre);

	// Ubica el nodo correspondiente. Si es el raiz, lo marca como 0. Si no existe, lo informa.
	if (strcmp(dir_padre, "/") == 0){
		nodo_padre = 0;
	} else if ((nodo_padre = determinar_nodo(dir_padre)) < 0){
		return -ENOENT;
	}

	node = node_table_start;

	// Toma un lock de escritura.
			log_lock_trace(logger, "Mkdir: Pide lock escritura. Escribiendo: %d. En cola: %d.", rwlock.__data.__writer, rwlock.__data.__nr_writers_queued);
	pthread_rwlock_wrlock(&rwlock);
			log_lock_trace(logger, "Mkdir: Recibe lock escritura.");
	// Abrir conexion y traer directorios, guarda el bloque de inicio para luego liberar memoria

	// Busca el primer nodo libre (state 0) y cuando lo encuentra, lo crea:
	for (i = 0; (node->state != 0) & (i <= NODE_TABLE_SIZE); i++) node = &(node_table_start[i]);
	// Si no hay un nodo libre, devuelve un error.
	if (i > NODE_TABLE_SIZE){
		res = -EDQUOT;
		goto finalizar;
	}

	// Escribe datos del archivo
	node->state = DIRECTORY_T;
	strcpy((char*) &(node->fname[0]), nombre);
	node->file_size = 0;
	node->parent_dir_block = nodo_padre;
	res = 0;

	finalizar:
	free(nom_to_free);
	free(dir_to_free);

	// Devuelve el lock de escritura.
	pthread_rwlock_unlock(&rwlock);
			log_lock_trace(logger, "Mkdir: Devuelve lock escritura. En cola: %d", rwlock.__data.__nr_writers_queued);
	return res;

}


/*
 *	@DESC
 *		Funcion que borra directorios de fuse.
 *
 *	@PARAM
 *		Path - El path donde tiene que borrar.
 *
 *	@RET
 *		0 Si esta OK, -ENOENT si no pudo.
 *
 */
int grasa_rmdir (const char* path){
			log_trace(logger, "Rmdir: Path: %s", path);
	int nodo_padre = determinar_nodo(path), i, res = 0;
	if (nodo_padre == -1) return -ENOENT;
	struct grasa_file_t *node;

	// Toma un lock de escritura.
			log_lock_trace(logger, "Rmdir: Pide lock escritura. Escribiendo: %d. En cola: %d.", rwlock.__data.__writer, rwlock.__data.__nr_writers_queued);
	pthread_rwlock_wrlock(&rwlock);
			log_lock_trace(logger, "Rmdir: Recibe lock escritura.");
	// Abre conexiones y levanta la tabla de nodos en memoria.
	node = &(node_table_start[-1]);

	node = &(node[nodo_padre]);

	// Chequea si el directorio esta vacio. En caso que eso suceda, FUSE se encarga de borrar lo que hay dentro.
	for (i=0; i < 1024 ;i++){
		if (((&node_table_start[i])->state != DELETED_T) & ((&node_table_start[i])->parent_dir_block == nodo_padre)) {
			res = -ENOTEMPTY;
			goto finalizar;
		}
	}

	node->state = DELETED_T; // Aca le dice que el estado queda "Borrado"


	// Cierra, ponele la alarma y se va para su casa. Mejor dicho, retorna 0 :D
	finalizar:
	// Devuelve el lock de escritura.
	pthread_rwlock_unlock(&rwlock);
			log_lock_trace(logger, "Rmdir: Devuelve lock escritura. En cola: %d", rwlock.__data.__nr_writers_queued);
	// Borra la estructura de cache:
	name_cache_free(&node_cache);
	return res;
}

/*
 * 	@DESC
 * 		Trunca un archivo a un length correspondiente. El archivo puede perder datos
 *
 * 	@PARAM
 * 		path - La ruta del archivo.
 * 		new_size - El nuevo size.
 *
 * 	@RET
 * 		Como siempre, 0 si esta OK.
 *
 */
int grasa_truncate (const char *path, off_t new_size){
			log_info(logger, "Truncate: Path: %s - New size: %d", path, new_size);
	if (new_size < 0) return -EINVAL; /* New File Size negativo */
	int nodo_padre = determinar_nodo(path);
	if (nodo_padre == -1) return -ENOENT;
	struct grasa_file_t *node;
	int res = 0;

	// Toma un lock de escritura.
			log_lock_trace(logger, "Truncate: Pide lock escritura. Escribiendo: %d. En cola: %d.", rwlock.__data.__writer, rwlock.__data.__nr_writers_queued);
	pthread_rwlock_wrlock(&rwlock);
			log_lock_trace(logger, "Truncate: Recibe lock escritura.");
	// Abre conexiones y levanta la tabla de nodos en memoria.
	node = node_table_start;

	node = &(node[nodo_padre-1]);

	// Si el nuevo size es mayor, se deben reservar los nodos correspondientes:
	if (new_size > node->file_size){
		res = get_new_space(node, (new_size - node->file_size));
		if (res != 0) goto finalizar;

	} else {	// Si no, se deben borrar los nodos hasta ese punto.
		int pointer_to_delete;
		int data_to_delete;

		set_position(&pointer_to_delete, &data_to_delete, 0, new_size);

		res = delete_nodes_upto(node, pointer_to_delete, data_to_delete);
		if(res != 0) goto finalizar;
	}

	node->file_size = new_size; // Aca le dice su nuevo size.


	finalizar:
	// Cierra, ponele la alarma y se va para su casa. Mejor dicho, retorna 0 :D
	// Devuelve el lock de escritura.
	pthread_rwlock_wrlock(&rwlock);
			log_lock_trace(logger, "Truncate: Devuelve lock escritura. En cola: %d", rwlock.__data.__nr_writers_queued);
	return res;
}


/*
 * 	@DESC
 * 		Funcion que escribe archivos en fuse. Tiene la posta.
 *
 * 	@PARAM
 * 		path - Dir del archivo
 * 		buf - Buffer que indica que datos copiar.
 * 		size - Tam de los datos a copiar
 * 		offset - Situa una posicion sobre la cual empezar a copiar datos
 * 		fi - File Info. Contiene flags y otras cosas locas que no hay que usar
 *
 * 	@RET
 * 		Devuelve la cantidad de bytes escritos, siempre y cuando este OK. Caso contrario, numero negativo tipo -ENOENT.
 */
int grasa_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
			log_trace(logger, "Writing: Path: %s - Size: %d - Offset %d", path, size, offset);
	(void) fi;
	int nodo = determinar_nodo(path);
	if (nodo == -1) return -ENOENT;
	int new_free_node;
	struct grasa_file_t *node;
	char *data_block;
	size_t tam = size, file_size, space_in_block, offset_in_block = offset % BLOCKSIZE;
	off_t off = offset;
	int *n_pointer_block = malloc(sizeof(int)), *n_data_block = malloc(sizeof(int));
	ptrGBloque *pointer_block;
	int res = size;

	// Ubica el nodo correspondiente al archivo
	node = &(node_table_start[nodo-1]);
	file_size = node->file_size;

	if ((file_size + size) >= THELARGESTFILE) return -EFBIG;

	// Toma un lock de escritura.
			log_lock_trace(logger, "Write: Pide lock escritura. Escribiendo: %d. En cola: %d.", rwlock.__data.__writer, rwlock.__data.__nr_writers_queued);
	pthread_rwlock_wrlock(&rwlock);
			log_lock_trace(logger, "Write: Recibe lock escritura.");

	// Guarda tantas veces como sea necesario, consigue nodos y actualiza el archivo.
	while (tam != 0){

		// Actualiza los valores de espacio restante en bloque.
		space_in_block = BLOCKSIZE - (file_size % BLOCKSIZE);
		if (space_in_block == BLOCKSIZE) (space_in_block = 0); // Porque significa que el bloque esta lleno.
		if (file_size == 0) space_in_block = BLOCKSIZE; /* Significa que el archivo esta recien creado y ya tiene un bloque de datos asignado */

		// Si el offset es mayor que el tamanio del archivo mas el resto del bloque libre, significa que hay que pedir un bloque nuevo
		// file_size == 0 indica que es un archivo que recien se comienza a escribir, por lo que tiene un tratamiento distinto (ya tiene un bloque de datos asignado).
		if ((off >= (file_size + space_in_block)) & (file_size != 0)){

			// Si no hay espacio en el disco, retorna error.
			if (bitmap_free_blocks == 0) return -ENOSPC;

			// Obtiene un bloque libre para escribir.
			new_free_node = get_node();
			if (new_free_node < 0) goto finalizar;

			// Agrega el nodo al archivo.
			res = add_node(node, new_free_node);
			if (res != 0) goto finalizar;

			// Lo relativiza al data block.
			new_free_node -= (GHEADERBLOCKS + NODE_TABLE_SIZE + BITMAP_BLOCK_SIZE);
			data_block = (char*) &(data_block_start[new_free_node]);

			// Actualiza el espacio libre en bloque.
			space_in_block = BLOCKSIZE;

		} else {
			// Ubica a que nodo le corresponderia guardar el dato
			set_position(n_pointer_block, n_data_block, file_size, off);

			//Ubica el nodo a escribir.
			*n_pointer_block = node->blk_indirect[*n_pointer_block];
			*n_pointer_block -= (GHEADERBLOCKS + NODE_TABLE_SIZE + BITMAP_BLOCK_SIZE);
			pointer_block = (ptrGBloque*) &(data_block_start[*n_pointer_block]);
			*n_data_block = pointer_block[*n_data_block];
			*n_data_block -= (GHEADERBLOCKS + NODE_TABLE_SIZE + BITMAP_BLOCK_SIZE);
			data_block = (char*) &(data_block_start[*n_data_block]);
		}

		// Escribe en ese bloque de datos.
		if (tam >= BLOCKSIZE){
			memcpy(data_block, buf, BLOCKSIZE);
			if ((node->file_size) <= (off)) file_size = node->file_size += BLOCKSIZE;
			buf += BLOCKSIZE;
			off += BLOCKSIZE;
			tam -= BLOCKSIZE;
			offset_in_block = 0;
		} else if (tam <= space_in_block){ /*Hay lugar suficiente en ese bloque para escribir el resto del archivo */
			memcpy(data_block + offset_in_block, buf, tam);
			if (node->file_size <= off) file_size = node->file_size += tam;
			else if (node->file_size <= (off + tam)) file_size = node->file_size += (off + tam - node->file_size);
			tam = 0;
		} else { /* Como no hay lugar suficiente, llena el bloque y vuelve a buscar uno nuevo */
			memcpy(data_block + offset_in_block, buf, space_in_block);
			file_size = node->file_size += space_in_block;
			buf += space_in_block;
			off += space_in_block;
			tam -= space_in_block;
			offset_in_block = 0;
		}

	}

	node->m_date= time(NULL);

	res = size;

	finalizar:
	// Devuelve el lock de escritura.
	pthread_rwlock_unlock(&rwlock);
			log_lock_trace(logger, "Write: Devuelve lock escritura. En cola: %d", rwlock.__data.__nr_writers_queued);
			log_trace(logger, "Terminada escritura.");
	return res;

}

/*
 *  @DESC
 *  	Se invoca esta funcion cada vez que fuse quiere hacer un archivo nuevo
 *
 *  @PARAM
 *  	path - Como siempre, el path del archivo relativo al disco
 *  	mode - Opciones del archivo
 *  	dev - Otra cosa que no se usa :D
 *
 *  @RET
 *  	Devuelve 0 si le sale OK, num negativo si no.
 */
int grasa_mknod (const char* path, mode_t mode, dev_t dev){
	if (determinar_nodo(path) != -1) return -EEXIST;
		log_info(logger, "Mknod: Path: %s", path);
	int nodo_padre, i, res = 0;
	int new_free_node;
	struct grasa_file_t *node;
	char *nombre = malloc(strlen(path) + 1), *nom_to_free = nombre;
	char *dir_padre = malloc(strlen(path) + 1), *dir_to_free = dir_padre;
	char *data_block;

	split_path(path, &dir_padre, &nombre);

	// Ubica el nodo correspondiente. Si es el raiz, lo marca como 0, Si es menor a 0, lo crea (mismos permisos).
	if (strcmp(dir_padre, "/") == 0) nodo_padre = 0;
	else if ((nodo_padre = determinar_nodo(dir_padre)) < 0) return -ENOENT;


	node = node_table_start;

	// Toma un lock de escritura.
			log_lock_trace(logger, "Mknod: Pide lock escritura. Escribiendo: %d. En cola: %d.", rwlock.__data.__writer, rwlock.__data.__nr_writers_queued);
	pthread_rwlock_wrlock(&rwlock);
			log_lock_trace(logger, "Mknod: Recibe lock escritura.");

	// Busca el primer nodo libre (state 0) y cuando lo encuentra, lo crea:
	for (i = 0; (node->state != 0) & (i <= NODE_TABLE_SIZE); i++) node = &(node_table_start[i]);
	// Si no hay un nodo libre, devuelve un error.
	if (i > NODE_TABLE_SIZE){
		res = -EDQUOT;
		goto finalizar;
	}

	// Escribe datos del archivo
	node->state = FILE_T;
	strcpy((char*) &(node->fname[0]), nombre);
	node->file_size = 0; // El tamanio se ira sumando a medida que se escriba en el archivo.
	node->parent_dir_block = nodo_padre;
	node->blk_indirect[0] = 0; // Se utiliza esta marca para avisar que es un archivo nuevo. De esta manera, la funcion add_node conoce que esta recien creado.
	node->c_date = node->m_date = time(NULL);
	res = 0;

	// Obtiene un bloque libre para escribir.
	new_free_node = get_node();

	// Actualiza la informacion del archivo.
	add_node(node, new_free_node);

	// Lo relativiza al data block.
	new_free_node -= (GHEADERBLOCKS + NODE_TABLE_SIZE + BITMAP_BLOCK_SIZE);
	data_block = (char*) &(data_block_start[new_free_node]);

	// Escribe en ese bloque de datos.
	memset(data_block, '\0', BLOCKSIZE);

	finalizar:
	free(nom_to_free);
	free(dir_to_free);

	// Devuelve el lock de escritura.
	pthread_rwlock_unlock(&rwlock);
			log_lock_trace(logger, "Mknod: Devuelve lock escritura. En cola: %d", rwlock.__data.__nr_writers_queued);
	return res;
}

/*
 *  @DESC
 *  	Funcion que se llama cuando hay que borrar un archivo
 *
 *  @PARAM
 *  	path - La ruta del archivo a borrar.
 *
 *  @RET
 *  	0 si salio bien
 *  	Numero negativo, si no
 */
int grasa_unlink (const char* path){
	struct grasa_file_t* file_data;
	int node = determinar_nodo(path);

	ENABLE_DELETE_MODE;

	file_data = &(node_table_start[node - 1]);

	// Toma un lock de escritura.
			log_lock_trace(logger, "Ulink: Pide lock escritura. Escribiendo: %d. En cola: %d.", rwlock.__data.__writer, rwlock.__data.__nr_writers_queued);
	pthread_rwlock_wrlock(&rwlock);
			log_lock_trace(logger, "Ulink: Recibe lock escritura.");

	delete_nodes_upto(file_data, 0, 0);

	// Devuelve el lock de escritura.
	pthread_rwlock_unlock(&rwlock);
			log_lock_trace(logger, "Ulink: Devuelve lock escritura. En cola: %d", rwlock.__data.__nr_writers_queued);
	// Borra la estructura de cache:
	name_cache_free(&node_cache);
	DISABLE_DELETE_MODE;

	return grasa_rmdir(path);
	}

/*
 *
 */
int grasa_rename (const char* oldpath, const char* newpath){
	if (determinar_nodo(oldpath) == -1) return -ENOENT;
			log_info(logger, "Rename: Moviendo archivo. From: %s - To: %s", oldpath, newpath);
	char* newroute = malloc(strlen(newpath) + 1);
	char* newname = malloc(GFILENAMELENGTH + 1);
	char* tofree1 = newroute;
	char* tofree2 = newname;
	split_path(newpath, &newroute, &newname);
	int old_node = determinar_nodo(oldpath), new_parent_node = determinar_nodo(newroute);


	// Toma un lock de escritura.
			log_lock_trace(logger, "Rename: Pide lock escritura. Escribiendo: %d. En cola: %d.", rwlock.__data.__writer, rwlock.__data.__nr_writers_queued);
	pthread_rwlock_wrlock(&rwlock);
			log_lock_trace(logger, "Rename: Recibe lock escritura.");

	// Modifica los valores del file. Como el determinar_nodo devuelve el numero de nodo +1, lo reubica.
	strcpy((char *) &(node_table_start[old_node - 1].fname[0]), newname);
	node_table_start[old_node -1].parent_dir_block = new_parent_node;

	// Devuelve el lock de escritura.
	pthread_rwlock_unlock(&rwlock);
			log_lock_trace(logger, "Rename: Devuelve lock escritura. En cola: %d", rwlock.__data.__nr_writers_queued);


	free(tofree1);
	free(tofree2);
	// Borra la estructura de cache:
	name_cache_free(&node_cache);

	return 0;
}

/*
 *	@DESC
 *		Algunas implementaciones de FUSE llaman a esta funcion para hacer el setteo de attributes.
 *
 *	@PARAM
 *		path - Ruta del archivo
 *		name - Nombre del archivo (puede cambiarse)
 *		value - Unknown
 *		size - Tamanio del archivo
 *		flags - Banderas que indican el estado del archivo
 *
 *	@RET
 *		0 si salio bien
 *		negativo - Rompio algo.
 */
int grasa_setxattr(const char* path, const char* name, const char* value, size_t size, int flags){
	int node_number = determinar_nodo(path);
	struct grasa_file_t *node = &(node_table_start[node_number]);
	char *nombre, *super_path;
	split_path(path, &super_path, &nombre);
	if (strcmp(name,nombre) != 0) grasa_rename(path, strcat(super_path, name));

	// Chequea el size del archivo.
	if (node->file_size != size) {
		pthread_rwlock_wrlock(&rwlock);
		node->file_size = size;
		pthread_rwlock_unlock(&rwlock);
	}

	return 0;
}

/*
 * 	@DESC
 * 		Cambia los tiempos de acceso y modificacion de un archivo
 *
 * 	@PARAM
 * 		path - Ruta del archivo.
 * 		timeBuf - Buffer que contiene los timers.
 *
 * 	@RETURN
 * 		0 - Funciona.
 * 		Negativo - Rompe.
 */
int grasa_utime(const char *path, struct utimbuf *timeBuf){
	int node_number = determinar_nodo(path);
	if (node_number == -1) return -ENOENT;
	struct grasa_file_t *node = &(node_table_start[node_number]);

	/* Si el buffer es nulo, entonces ambos tiempos deben ser cargados con la hora actual */
	if (timeBuf == NULL){
		/* Time devuelve la hora actual */
		node->m_date = time(NULL); /* No lockeamos porque, el ultimo que cae, es el que le corresponde dejar la hora */
		return 0;
	}

	node->m_date = timeBuf->modtime;
	/* GRASA no tiene definidos cambios en el Access Time */
	return 0;
}

/*
 *
 */
int grasa_flush(const char* path, struct fuse_file_info *fi){

	// Toma un lock de escritura.
			log_lock_trace(logger, "Flush: Pide lock escritura. Escribiendo: %d. En cola: %d.", rwlock.__data.__writer, rwlock.__data.__nr_writers_queued);
	pthread_rwlock_wrlock(&rwlock);
			log_lock_trace(logger, "Flush: Recibe lock escritura.");

	fdatasync(discDescriptor);

	// Devuelve el lock de escritura.
	pthread_rwlock_unlock(&rwlock);
			log_lock_trace(logger, "Flush: Devuelve lock escritura. En cola: %d", rwlock.__data.__nr_writers_queued);

	// Borra la estructura de cache:
	name_cache_free(&node_cache);

	return 0;
}
