# Class graph
Experimento en c para obtener un listado de clases php contenidas en un arbol de directorios para poder generar un gráfico de dependencias utilizando la sintaxis .dot de Graphviz

Uso: 
`cg -i "directorio_base" -o "nombre_fichero_lista" -g "nombre_fichero_grafico" -x "patron1;patron2;patron3"`

Parámetros:<br/>
-i directorio a partir del cual se hce el análisis <br/>
-o nombre del fichero para el listado de clases <br/>
-g nombre del fichero .dot para el gráfico generado <br/>
-x patrones para excluir subdirectorios separados por ; <br/>


Ejemplo: 
`cg -i "c:/pro/proyecto1" -o "list.txt" -g "grafico.dot" -x "\*/libs;\*/aws"`

> [!NOTE]
> Para poder trabajar con el fichero .dot es necesario tener disponible el programa graphviz disponible en https://graphviz.org/download/
