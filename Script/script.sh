#!/bin/bash

#/*
# * chatterbox Progetto del corso di LSO 2017
# *
# * Dipartimento di Informatica Università di Pisa
# * Docenti: Prencipe, Torquati
# *
# * Studente: Luca Corbucci
# * Matricola: 516450
# *
# */

function getPath {
  grep -v '^#' $dirname | grep DirName | cut -f 2 -d "="

}

function usage {
  echo "$0 : Inserire due parametri: il nome del file di configurazione ed un numero intero positivo.
Verranno rimossi gli elementi presenti nel path specificato nel file di configurazione più vecchi dell'intero indicato" 1>&2
}

#-------------------------------------------------------#

# Controlliamo il numero dei parametri.
# Se ho 0 parametri eseguo la funzione usage che restituisce il messaggio di "aiuto".
# Se ho un solo parametro controllo che questo sia "-help", in questo caso eseguo usage e termino
# altrimenti termino.
# Se ho più di 1 parametri li controllo ad uno ad uno in modo da controllare se uno di
# questi parametri sia "-help". Se trovo "-help" eseguo la funzione usage.
# Se non trovo "-help" allora assegno alle variabili dirname e num i valori passati come parametro
if [ $# -eq 0 ]; then
  $(usage)
  exit
elif [ $# -eq 1 ]; then
  if [ "$1" == "-help" ]; then
    $(usage)
    exit
  else
    echo "Hai inserito un parametro, inserisci due parametri"
    exit
  fi

elif [ $# -gt 1 ]; then
  for i in "$@"; do
    if [ "$i" == "-help" ]; then
      $(usage)
      exit
    fi
  done

  # Controllo che il primo parametro, ovvero il nome del file di configurazione, esista.
  # Se esiste lo assegno alla variabile dirname ed estraggo dal file il path che mi interessa
  if [ -f $1 ]; then
    dirname=$1
    path=$(getPath)
  else
    echo "Inserire il nome di un file esistente"
    exit
  fi
  # Controllo che il numero passato come parametro sia davvero un numero.
  # Controllo che il numero passato come parametro sia maggiore di 0
  # Se è maggiore di 0 lo assegno a num
  if [ $2 -eq $2 2> /dev/null ]; then
    if [ $2 -lt 0 ]; then
      echo "Errore: inserire un numero positivo come secondo parametro."
      exit
    else
      num=$2
    fi
  else
    echo "Inserire un numero come secondo parametro"
    exit
  fi

fi

# Controllo l'esistenza della directory estratta dal file di configurazione
# Se la directory esiste allora controllo la variabile num.
# Se num è 0 allora stampo la lista dei file presenti in path.
# Se num è maggiore di 0 allora cerco nel path i file e le directory più vecchi di num minuti
if [ -d $path ]; then

  if [ $num -eq 0 ]; then
    ls $path
    exit
  else
    find $path -name "*" ! -path $path -mmin +$num | xargs rm -df
    #Possibile aggiunta: restituire il numero di elementi cancellati.
    #output=$(find $path -name "*" ! -path $path -mmin +$num | xargs rm -vdf | wc -l)
    #echo "Rimossi" $output "elementi"
  fi

else
  echo "Directory non esistente"
  exit

fi
