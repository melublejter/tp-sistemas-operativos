/*
 * Sistemas Operativos - Super Mario Proc RELOADED.
 * Grupo       : C o no ser.
 * Nombre      : protocolo.c.
 * Descripcion : Este archivo contiene la implementacion del protocolo de comunicacion entre procesos.
 */
#include "protocolo.h"

char *enumToString(tMensaje tipoDeMensaje){
	t_config *configProtocolo;
	configProtocolo = config_create("../Shared-Library/ginyu/protocolo.config");
	char **arrayTipoMensajes;
	char *msjReturn;
	arrayTipoMensajes = config_try_get_array_value(configProtocolo, "TiposDeMensajes");
	if(arrayTipoMensajes != NULL){
		msjReturn = arrayTipoMensajes[tipoDeMensaje];
	} else{
		msjReturn = NULL;
	}
	config_destroy(configProtocolo);
	return msjReturn;
}

int serializarHandshakePers(tMensaje tipoMensaje, tHandshakePers handshakePersonaje, tPaquete* pPaquete) {
	int offset   = 0;
	int tmp_size = 0;

	pPaquete->type = tipoMensaje;

	tmp_size = sizeof(handshakePersonaje.simbolo);
	memcpy(pPaquete->payload, &handshakePersonaje.simbolo, tmp_size);

	offset   = tmp_size;
	tmp_size = strlen(handshakePersonaje.nombreNivel) + 1;
	memcpy(pPaquete->payload + offset, handshakePersonaje.nombreNivel, tmp_size);

	pPaquete->length = offset + tmp_size;

	return EXIT_SUCCESS;
}

tHandshakePers* deserializarHandshakePers(char * payload) {
	tHandshakePers *pHandshakePersonaje = malloc(sizeof(tHandshakePers));
	int offset   = 0;
	int tmp_size = 0;

	tmp_size = sizeof(tSimbolo);
	memcpy(&pHandshakePersonaje->simbolo, payload, tmp_size);

	offset = tmp_size;
	for (tmp_size = 1; (payload + offset)[tmp_size-1] != '\0'; tmp_size++);
	pHandshakePersonaje->nombreNivel = malloc(tmp_size);
	memcpy(pHandshakePersonaje->nombreNivel, payload + offset, tmp_size);

	return pHandshakePersonaje;
}


int serializarInfoNivel(tMensaje tipoMensaje, tInfoNivel infoNivel, tPaquete* pPaquete) {
	int offset   = 0;
	int tmp_size = 0;

	pPaquete->type = tipoMensaje;

	tmp_size = sizeof(uint32_t);
	memcpy(pPaquete->payload, &infoNivel.delay, tmp_size);

	offset   = tmp_size;
	tmp_size = sizeof(int8_t);
	memcpy(pPaquete->payload + offset, &infoNivel.quantum, tmp_size);

	offset  += tmp_size;
	tmp_size = sizeof(tAlgoritmo);
	memcpy(pPaquete->payload + offset, &infoNivel.algoritmo, tmp_size);

	pPaquete->length = offset + tmp_size;

	return EXIT_SUCCESS;
}

tInfoNivel * deserializarInfoNivel(char * payload) {
	tInfoNivel *pInfoNivel = malloc(sizeof(tInfoNivel));
	int offset   = 0;
	int tmp_size = 0;

	tmp_size = sizeof(uint32_t);
	memcpy(&pInfoNivel->delay, payload, tmp_size);

	offset   = tmp_size;
	tmp_size = sizeof(int8_t);
	memcpy(&pInfoNivel->quantum, payload + offset, tmp_size);

	offset  += tmp_size;
	tmp_size = sizeof(tAlgoritmo);
	memcpy(&pInfoNivel->algoritmo, payload + offset, tmp_size);

	return pInfoNivel;
}


int serializarPregPosicion(tMensaje tipoMensaje, tPregPosicion pregPosicion, tPaquete* pPaquete) {
	int offset   = 0;
	int tmp_size = 0;

	pPaquete->type = tipoMensaje;

	tmp_size = sizeof(pregPosicion.recurso);
	memcpy(pPaquete->payload, &(pregPosicion.recurso), tmp_size);

	offset   = tmp_size;
	tmp_size = sizeof(pregPosicion.simbolo);
	memcpy(pPaquete->payload + offset, &(pregPosicion.simbolo), tmp_size);

	pPaquete->length = offset + tmp_size;

	return EXIT_SUCCESS;
}

tPregPosicion * deserializarPregPosicion(char * payload) {

	tPregPosicion *pPregPosicion = malloc(sizeof(tPregPosicion));
	int offset   = 0;
	int tmp_size = 0;

	tmp_size = sizeof(tSimbolo);
	memcpy(&pPregPosicion->recurso, payload, tmp_size);

	offset   = tmp_size;
	tmp_size = sizeof(tSimbolo);
	memcpy(&pPregPosicion->simbolo, payload + offset, tmp_size);

	return pPregPosicion;
}

int serializarRtaPosicion(tMensaje tipoMensaje, tRtaPosicion rtaPosicion, tPaquete* pPaquete) {

	int offset   = 0;
	int tmp_size = 0;

	pPaquete->type = tipoMensaje;

	tmp_size = sizeof(rtaPosicion.posX);
	memcpy(pPaquete->payload, &(rtaPosicion.posX), tmp_size);

	offset   = tmp_size;
	tmp_size = sizeof(rtaPosicion.posY);
	memcpy(pPaquete->payload + offset, &(rtaPosicion.posY), tmp_size);

	pPaquete->length = offset + tmp_size;

	return EXIT_SUCCESS;
}

tRtaPosicion * deserializarRtaPosicion(char * payload) {

	tRtaPosicion *pRtaPosicion = malloc(sizeof(tRtaPosicion));
	int offset   = 0;
	int tmp_size = 0;

	tmp_size = sizeof(pRtaPosicion->posX);
	memcpy(&pRtaPosicion->posX, payload, tmp_size);

	offset   = tmp_size;
	tmp_size = sizeof(pRtaPosicion->posY);
	memcpy(&pRtaPosicion->posY, payload + offset, tmp_size);

	return pRtaPosicion;
}

int serializarMovimientoPers(tMensaje tipoMensaje, tMovimientoPers movimientoPers, tPaquete* pPaquete){

	int tmp_size = 0, offset = 0;
	pPaquete->type = tipoMensaje;

	tmp_size = sizeof(movimientoPers.simbolo);
	memcpy(pPaquete->payload, &movimientoPers.simbolo, tmp_size);

	offset   = tmp_size;
	tmp_size = sizeof(movimientoPers.direccion);
	memcpy(pPaquete->payload + offset, &movimientoPers.direccion, tmp_size);

	pPaquete->length = offset + tmp_size;

	return EXIT_SUCCESS;

}

tMovimientoPers* deserializarMovimientoPers(char * payload){

	tMovimientoPers *movimientoPers = malloc(sizeof(tMovimientoPers));
	int tmp_size = 0, offset = 0;

	tmp_size = sizeof(movimientoPers->simbolo);
	memcpy(&movimientoPers->simbolo, payload, tmp_size);

	offset   = tmp_size;
	tmp_size = sizeof(movimientoPers->direccion);
	memcpy(&movimientoPers->direccion, payload + offset, tmp_size);

	return movimientoPers;
}

int serializarEstado(tMensaje tipoMensaje, tEstado estadoPersonaje, tPaquete* pPaquete) {

	pPaquete->type = tipoMensaje;

	pPaquete->length = sizeof(estadoPersonaje);
	memcpy(pPaquete->payload, &estadoPersonaje, pPaquete->length);

	return EXIT_SUCCESS;
}

tEstado* deserializarEstado(char * payload) {

	tEstado *pEstadoPersonaje = malloc(sizeof(tEstado));
	int tmp_size = 0;

	tmp_size = sizeof(int8_t);
	memcpy(pEstadoPersonaje, payload, tmp_size);

	return pEstadoPersonaje;
}


int serializarSimbolo(tMensaje tipoMensaje, tSimbolo simbolo, tPaquete* pPaquete) {

	pPaquete->type = tipoMensaje;

	pPaquete->length = sizeof(simbolo);
	memcpy(pPaquete->payload, &simbolo, pPaquete->length);

	return EXIT_SUCCESS;
}

tSimbolo* deserializarSimbolo(char * payload) {

	tSimbolo *pSimbolo = malloc(sizeof(tSimbolo));
	int tmp_size = 0;

	tmp_size = sizeof(tSimbolo);
	memcpy(pSimbolo, payload, tmp_size);

	return pSimbolo;
}

tDesconexionPers* deserializarPersDesconect(char *payload){
	int i,recs,offset=sizeof(tSimbolo);
	tDesconexionPers* persDesconectado=malloc(sizeof(tDesconexionPers));
	memcpy(&recs,payload+offset,sizeof(int8_t));
	memcpy(&persDesconectado->simbolo,payload,sizeof(tSimbolo));
	memcpy(&persDesconectado->lenghtRecursos,payload+offset,sizeof(int8_t));
	offset+=sizeof(int8_t);
	for(i=0;i<recs;i++){
		persDesconectado->recursos[i]=(int8_t)*(payload+offset);
		offset+=sizeof(int8_t);
	}
	return persDesconectado;
}

int serializarDesconexionPers(tMensaje tipoMensaje, tDesconexionPers descPers, tPaquete* pPaquete) {
	int offset   = 0;
	int tmp_size = 0;

	pPaquete->type = tipoMensaje;

	tmp_size = sizeof(tSimbolo);
	memcpy(pPaquete->payload, &descPers.simbolo, tmp_size);

	offset   = tmp_size;
	tmp_size = sizeof(int8_t);
	memcpy(pPaquete->payload + offset, &descPers.lenghtRecursos, tmp_size);

	offset  += tmp_size;
	tmp_size = descPers.lenghtRecursos*sizeof(int8_t);
	memcpy(pPaquete->payload + offset, &descPers.recursos, tmp_size);

	pPaquete->length = offset + tmp_size;

	return EXIT_SUCCESS;
}

tDesconexionPers * deserializarDesconexionPers(char * payload) {
	tDesconexionPers *descPers = malloc(strlen(payload)*sizeof(char));
	int offset   = 0;
	int tmp_size = 0;

	tmp_size = sizeof(int8_t);
	memcpy(&descPers->simbolo, payload, tmp_size);

	offset   = tmp_size;
	tmp_size = sizeof(int8_t);
	memcpy(&descPers->lenghtRecursos, payload + offset, tmp_size);

	offset  += tmp_size;
	tmp_size = descPers->lenghtRecursos*sizeof(int8_t);
	memcpy(&descPers->recursos, payload + offset, tmp_size);

	return descPers;
}

