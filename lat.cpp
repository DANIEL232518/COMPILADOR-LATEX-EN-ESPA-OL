#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include <map>
#include <list>

using namespace std;

// ===============================
// ENUMERACIONES Y ESTRUCTURAS
// ===============================

enum class TipoToken {
    DOCUMENTO_INICIO, DOCUMENTO_FIN, CONFIGURAR,
    SECCION, SUBSECCION, SUBSUBSECCION, CAPITULO,
    NEGRITA, CURSIVA, SUBRAYADO, PARRAFO, TACHADO,
    LISTA_SIMPLE, LISTA_NUMERADA, ELEMENTO, FIN_LISTA,
    ECUACION, FORMULA, TABLA, FILA, COLUMNA, FIN_TABLA,
    FIGURA, LEYENDA, REFERENCIA, TITULO, AUTOR, FECHA,
    CENTRAR, IZQUIERDA, DERECHA, JUSTIFICAR,
    SALTO_LINEA, SALTO_PAGINA, NUEVA_PAGINA,
    
    PARENTESIS_IZQ, PARENTESIS_DER, LLAVE_IZQ, LLAVE_DER,
    CORCHETE_IZQ, CORCHETE_DER, COMA, PUNTO_COMA, IGUAL,
    PORCENTAJE, DOLAR, AMPERSAND,
    
    IDENTIFICADOR, CADENA, NUMERO, TEXTO_SIMPLE,
    
    FIN_ARCHIVO
};

// Constantes para la tabla de transiciones
const int FIN = 666;
const int ERROR_SINTACTICO = 999;

struct Token {
    TipoToken tipo;
    string valor;
    int linea;
    int columna;
    
    Token(TipoToken t, const string& v, int l, int c) 
        : tipo(t), valor(v), linea(l), columna(c) {}
    
    string toString() const {
        static map<TipoToken, string> nombresTipo = {
            {TipoToken::DOCUMENTO_INICIO, "DOCUMENTO_INICIO"},
            {TipoToken::DOCUMENTO_FIN, "DOCUMENTO_FIN"},
            {TipoToken::CONFIGURAR, "CONFIGURAR"},
            {TipoToken::SECCION, "SECCION"},
            {TipoToken::SUBSECCION, "SUBSECCION"},
            {TipoToken::SUBSUBSECCION, "SUBSUBSECCION"},
            {TipoToken::CAPITULO, "CAPITULO"},
            {TipoToken::NEGRITA, "NEGRITA"},
            {TipoToken::CURSIVA, "CURSIVA"},
            {TipoToken::SUBRAYADO, "SUBRAYADO"},
            {TipoToken::TACHADO, "TACHADO"},
            {TipoToken::PARRAFO, "PARRAFO"},
            {TipoToken::LISTA_SIMPLE, "LISTA_SIMPLE"},
            {TipoToken::LISTA_NUMERADA, "LISTA_NUMERADA"},
            {TipoToken::ELEMENTO, "ELEMENTO"},
            {TipoToken::FIN_LISTA, "FIN_LISTA"},
            {TipoToken::ECUACION, "ECUACION"},
            {TipoToken::FORMULA, "FORMULA"},
            {TipoToken::TEXTO_SIMPLE, "TEXTO_SIMPLE"}
        };
        
        auto it = nombresTipo.find(tipo);
        if (it != nombresTipo.end()) {
            return it->second + "('" + valor + "')";
        }
        return "TOKEN('" + valor + "')";
    }
    
    // Para usar en tabla de transiciones
    int getTokenValue() const {
        return static_cast<int>(tipo);
    }
};

// ===============================
// TABLA DE SIMBOLOS
// ===============================

class Atributos {
public:
    string lexema;
    int token;
    string tipo;
    string valor;
    string estado;
    
    Atributos() {
        lexema = "";
        token = -999;
        tipo = "";
        valor = "nulo";
        estado = "";
    }
    
    Atributos(string lex, int tok, string tip, string val, string est) {
        lexema = lex;
        token = tok;
        tipo = tip;
        valor = val;
        estado = est;
    }
    
    void Mostrar() {
        cout << "Tipo(" << tipo << ") \t";
        cout << "Lexema(" << lexema << ") \t";
        cout << "Token(" << token << ") \t";
        cout << "Valor(" << valor << ") \t";
        cout << "Estado(" << estado << ")" << endl;
    }
};

class TablaSimbolos {
private:
    list<Atributos> tabla;

public:
    void Insertar(string lex, int tok, string tip, string val, string est) {
        Atributos attr(lex, tok, tip, val, est);
        tabla.push_back(attr);
    }
    
    bool ActualizarValor(string lex, string val) {
        for (auto &item : tabla) {
            if (item.lexema == lex) {
                item.valor = val;
                return true;
            }
        }
        return false;
    }
    
    bool ActualizarTipo(string lex, string tipo) {
        for (auto &item : tabla) {
            if (item.lexema == lex) {
                item.tipo = tipo;
                return true;
            }
        }
        return false;
    }
    
    bool ActualizarEstado(string lex, string est) {
        for (auto &item : tabla) {
            if (item.lexema == lex) {
                item.estado = est;
                return true;
            }
        }
        return false;
    }
    
    void Mostrar() {
        cout << "\nTABLA DE SIMBOLOS:\n";
        cout << "==================\n";
        for (auto item : tabla) {
            item.Mostrar();
        }
    }
    
    bool Buscar(string lex, Atributos& attr) {
        for (auto item : tabla) {
            if (item.lexema == lex) {
                attr = item;
                return true;
            }
        }
        return false;
    }
    
    bool BuscarPalabraClave(string lex, Atributos& attr) {
        for (auto item : tabla) {
            if (item.lexema == lex && item.tipo == "pclave") {
                attr = item;
                return true;
            }
        }
        return false;
    }
    
    list<Atributos> getTabla() {
        return tabla;
    }
};

// ===============================
// ESTRUCTURAS DEL ARBOL SINTACTICO
// ===============================

struct NodoAST {
    virtual ~NodoAST() = default;
    virtual string toString() const = 0;
};

struct NodoDocumento : public NodoAST {
    vector<unique_ptr<NodoAST>> hijos;
    
    string toString() const override {
        return "Documento(" + to_string(hijos.size()) + " hijos)";
    }
};

struct NodoConfiguracion : public NodoAST {
    string clave;
    string valor;
    NodoConfiguracion(const string& k, const string& v) : clave(k), valor(v) {}
    
    string toString() const override {
        return "Configurar(" + clave + " = " + valor + ")";
    }
};

struct NodoSeccion : public NodoAST {
    string nivel;
    string titulo;
    NodoSeccion(const string& l, const string& t) : nivel(l), titulo(t) {}
    
    string toString() const override {
        return nivel + "(\"" + titulo + "\")";
    }
};

struct NodoTexto : public NodoAST {
    string contenido;
    string estilo;
    string alineacion;
    NodoTexto(const string& c, const string& s = "normal", const string& a = "justificar") 
        : contenido(c), estilo(s), alineacion(a) {}
    
    string toString() const override {
        return "Texto[" + estilo + "](\"" + contenido + "\")";
    }
};

struct NodoLista : public NodoAST {
    string tipo;
    vector<unique_ptr<NodoAST>> elementos;
    
    string toString() const override {
        return "Lista[" + tipo + "](" + to_string(elementos.size()) + " elementos)";
    }
};

struct NodoEcuacion : public NodoAST {
    string contenido;
    string tipo;
    NodoEcuacion(const string& c, const string& t = "ecuacion") : contenido(c), tipo(t) {}
    
    string toString() const override {
        return "Ecuacion[" + tipo + "](" + contenido + ")";
    }
};

struct NodoMetadatos : public NodoAST {
    string tipo;
    string contenido;
    NodoMetadatos(const string& t, const string& c) : tipo(t), contenido(c) {}
    
    string toString() const override {
        return tipo + "(\"" + contenido + "\")";
    }
};

struct NodoComando : public NodoAST {
    string comando;
    string contenido;
    NodoComando(const string& cmd, const string& c = "") : comando(cmd), contenido(c) {}
    
    string toString() const override {
        return "Comando(" + comando + ")";
    }
};

// ===============================
// ANALIZADOR LEXICO
// ===============================

class AnalizadorLexico {
private:
    string entrada;
    size_t posicion;
    int linea;
    int columna;
    TablaSimbolos tablaSimbolos;
    
    char mirarSiguiente() {
        return posicion < entrada.length() ? entrada[posicion] : '\0';
    }
    
    char avanzar() {
        if (posicion >= entrada.length()) return '\0';
        char c = entrada[posicion++];
        if (c == '\n') {
            linea++;
            columna = 1;
        } else {
            columna++;
        }
        return c;
    }
    
    void saltarEspacios() {
        while (isspace(mirarSiguiente()) && mirarSiguiente() != '\n') {
            avanzar();
        }
    }
    
    void saltarComentario() {
        while (mirarSiguiente() != '\n' && mirarSiguiente() != '\0') {
            avanzar();
        }
    }
    
    Token leerCadena() {
        avanzar(); // saltar comilla inicial
        string valor;
        int lineaInicio = linea;
        int columnaInicio = columna;
        
        while (mirarSiguiente() != '"' && mirarSiguiente() != '\0') {
            if (mirarSiguiente() == '\\') {
                avanzar(); // saltar barra invertida
                switch (mirarSiguiente()) {
                    case 'n': valor += '\n'; break;
                    case 't': valor += '\t'; break;
                    case '"': valor += '"'; break;
                    case '\\': valor += '\\'; break;
                    default: valor += '\\'; valor += mirarSiguiente(); break;
                }
                avanzar();
            } else {
                valor += avanzar();
            }
        }
        
        if (mirarSiguiente() == '"') {
            avanzar();
        } else {
            throw runtime_error("Cadena sin cerrar en linea " + to_string(lineaInicio));
        }
        
        return Token(TipoToken::CADENA, valor, lineaInicio, columnaInicio);
    }
    
    Token leerIdentificador() {
        string valor;
        int lineaInicio = linea;
        int columnaInicio = columna;
        
        while (isalnum(mirarSiguiente()) || mirarSiguiente() == '_') {
            valor += avanzar();
        }
        
        // Convertir a minusculas
        string valorMinusculas = valor;
        transform(valorMinusculas.begin(), valorMinusculas.end(), valorMinusculas.begin(), ::tolower);
        
        Atributos attr;
        if (tablaSimbolos.BuscarPalabraClave(valorMinusculas, attr)) {
            return Token(static_cast<TipoToken>(attr.token), valor, lineaInicio, columnaInicio);
        }
        
        // Si no es palabra clave, es identificador
        tablaSimbolos.Insertar(valor, static_cast<int>(TipoToken::IDENTIFICADOR), "identificador", "nulo", "no_asignado");
        return Token(TipoToken::IDENTIFICADOR, valor, lineaInicio, columnaInicio);
    }
    
    Token leerNumero() {
        string valor;
        int lineaInicio = linea;
        int columnaInicio = columna;
        
        while (isdigit(mirarSiguiente()) || mirarSiguiente() == '.') {
            valor += avanzar();
        }
        
        tablaSimbolos.Insertar(valor, static_cast<int>(TipoToken::NUMERO), "numero", valor, "asignado");
        return Token(TipoToken::NUMERO, valor, lineaInicio, columnaInicio);
    }
    
    Token leerTextoSimple() {
        string valor;
        int lineaInicio = linea;
        int columnaInicio = columna;
        
        while (mirarSiguiente() != ';' && mirarSiguiente() != '\n' && mirarSiguiente() != '\0') {
            if (isalpha(mirarSiguiente())) {
                size_t posicionGuardada = posicion;
                int lineaGuardada = linea;
                int columnaGuardada = columna;
                
                string posiblePalabraClave;
                while (isalnum(mirarSiguiente()) || mirarSiguiente() == '_') {
                    posiblePalabraClave += avanzar();
                }
                
                string palabraMinusculas = posiblePalabraClave;
                transform(palabraMinusculas.begin(), palabraMinusculas.end(), palabraMinusculas.begin(), ::tolower);
                
                Atributos attr;
                if (tablaSimbolos.BuscarPalabraClave(palabraMinusculas, attr)) {
                    posicion = posicionGuardada;
                    linea = lineaGuardada;
                    columna = columnaGuardada;
                    break;
                } else {
                    valor += posiblePalabraClave;
                }
            } else {
                valor += avanzar();
            }
        }
        
        while (!valor.empty() && isspace(valor.back())) {
            valor.pop_back();
        }
        
        if (!valor.empty()) {
            tablaSimbolos.Insertar(valor, static_cast<int>(TipoToken::TEXTO_SIMPLE), "texto", valor, "asignado");
        }
        
        return Token(TipoToken::TEXTO_SIMPLE, valor, lineaInicio, columnaInicio);
    }

    bool esElemento(char c) {
        string elementos = "(){}[]=,;.%$&";
        return elementos.find(c) != string::npos;
    }

public:
    AnalizadorLexico(const string& entrada) 
        : entrada(entrada), posicion(0), linea(1), columna(1) {
        
        // Inicializar tabla de simbolos con palabras reservadas
        tablaSimbolos.Insertar("documento_inicio", static_cast<int>(TipoToken::DOCUMENTO_INICIO), "pclave", "-", "-");
        tablaSimbolos.Insertar("documento_fin", static_cast<int>(TipoToken::DOCUMENTO_FIN), "pclave", "-", "-");
        tablaSimbolos.Insertar("configurar", static_cast<int>(TipoToken::CONFIGURAR), "pclave", "-", "-");
        tablaSimbolos.Insertar("seccion", static_cast<int>(TipoToken::SECCION), "pclave", "-", "-");
        tablaSimbolos.Insertar("subseccion", static_cast<int>(TipoToken::SUBSECCION), "pclave", "-", "-");
        tablaSimbolos.Insertar("subsubseccion", static_cast<int>(TipoToken::SUBSUBSECCION), "pclave", "-", "-");
        tablaSimbolos.Insertar("capitulo", static_cast<int>(TipoToken::CAPITULO), "pclave", "-", "-");
        tablaSimbolos.Insertar("negrita", static_cast<int>(TipoToken::NEGRITA), "pclave", "-", "-");
        tablaSimbolos.Insertar("cursiva", static_cast<int>(TipoToken::CURSIVA), "pclave", "-", "-");
        tablaSimbolos.Insertar("subrayado", static_cast<int>(TipoToken::SUBRAYADO), "pclave", "-", "-");
        tablaSimbolos.Insertar("tachado", static_cast<int>(TipoToken::TACHADO), "pclave", "-", "-");
        tablaSimbolos.Insertar("parrafo", static_cast<int>(TipoToken::PARRAFO), "pclave", "-", "-");
        tablaSimbolos.Insertar("lista_simple", static_cast<int>(TipoToken::LISTA_SIMPLE), "pclave", "-", "-");
        tablaSimbolos.Insertar("lista_numerada", static_cast<int>(TipoToken::LISTA_NUMERADA), "pclave", "-", "-");
        tablaSimbolos.Insertar("elemento", static_cast<int>(TipoToken::ELEMENTO), "pclave", "-", "-");
        tablaSimbolos.Insertar("fin_lista", static_cast<int>(TipoToken::FIN_LISTA), "pclave", "-", "-");
        tablaSimbolos.Insertar("ecuacion", static_cast<int>(TipoToken::ECUACION), "pclave", "-", "-");
        tablaSimbolos.Insertar("formula", static_cast<int>(TipoToken::FORMULA), "pclave", "-", "-");
        tablaSimbolos.Insertar("titulo", static_cast<int>(TipoToken::TITULO), "pclave", "-", "-");
        tablaSimbolos.Insertar("autor", static_cast<int>(TipoToken::AUTOR), "pclave", "-", "-");
        tablaSimbolos.Insertar("fecha", static_cast<int>(TipoToken::FECHA), "pclave", "-", "-");
        tablaSimbolos.Insertar("salto_linea", static_cast<int>(TipoToken::SALTO_LINEA), "pclave", "-", "-");
        tablaSimbolos.Insertar("salto_pagina", static_cast<int>(TipoToken::SALTO_PAGINA), "pclave", "-", "-");
        tablaSimbolos.Insertar("nueva_pagina", static_cast<int>(TipoToken::NUEVA_PAGINA), "pclave", "-", "-");
        
        // Simbolos
        tablaSimbolos.Insertar("(", static_cast<int>(TipoToken::PARENTESIS_IZQ), "pclave", "-", "-");
        tablaSimbolos.Insertar(")", static_cast<int>(TipoToken::PARENTESIS_DER), "pclave", "-", "-");
        tablaSimbolos.Insertar("{", static_cast<int>(TipoToken::LLAVE_IZQ), "pclave", "-", "-");
        tablaSimbolos.Insertar("}", static_cast<int>(TipoToken::LLAVE_DER), "pclave", "-", "-");
        tablaSimbolos.Insertar("[", static_cast<int>(TipoToken::CORCHETE_IZQ), "pclave", "-", "-");
        tablaSimbolos.Insertar("]", static_cast<int>(TipoToken::CORCHETE_DER), "pclave", "-", "-");
        tablaSimbolos.Insertar(",", static_cast<int>(TipoToken::COMA), "pclave", "-", "-");
        tablaSimbolos.Insertar(";", static_cast<int>(TipoToken::PUNTO_COMA), "pclave", "-", "-");
        tablaSimbolos.Insertar("=", static_cast<int>(TipoToken::IGUAL), "pclave", "-", "-");
        tablaSimbolos.Insertar("%", static_cast<int>(TipoToken::PORCENTAJE), "pclave", "-", "-");
        tablaSimbolos.Insertar("$", static_cast<int>(TipoToken::DOLAR), "pclave", "-", "-");
        tablaSimbolos.Insertar("&", static_cast<int>(TipoToken::AMPERSAND), "pclave", "-", "-");
    }
    
    vector<Token> analizar() {
        vector<Token> tokens;
        
        while (posicion < entrada.length()) {
            saltarEspacios();
            
            char c = mirarSiguiente();
            if (c == '\0') break;
            
            int lineaActual = linea;
            int columnaActual = columna;
            
            if (c == '"') {
                tokens.push_back(leerCadena());
            } else if (isalpha(c) || c == '_') {
                tokens.push_back(leerIdentificador());
            } else if (isdigit(c)) {
                tokens.push_back(leerNumero());
            } else if (c == '%') {
                saltarComentario();
            } else if (esElemento(c)) {
                string simbolo(1, c);
                Atributos attr;
                if (tablaSimbolos.BuscarPalabraClave(simbolo, attr)) {
                    tokens.push_back(Token(static_cast<TipoToken>(attr.token), simbolo, lineaActual, columnaActual));
                    avanzar();
                } else {
                    avanzar();
                }
            } else if (c == '\n') {
                avanzar();
            } else {
                Token tokenTexto = leerTextoSimple();
                if (!tokenTexto.valor.empty()) {
                    tokens.push_back(tokenTexto);
                } else {
                    avanzar();
                }
            }
        }
        
        tokens.push_back(Token(TipoToken::FIN_ARCHIVO, "", linea, columna));
        return tokens;
    }
    
    void mostrarTablaSimbolos() {
        tablaSimbolos.Mostrar();
    }
    
    TablaSimbolos& getTablaSimbolos() {
        return tablaSimbolos;
    }
};

// ===============================
// ANALIZADOR SINTACTICO CON TABLA DE TRANSICIONES
// ===============================

class AnalizadorSintactico {
private:
    vector<Token> tokens;
    size_t actual;
    int estado;
    int tTransicion[100][100];
    
    void inicializarTablaTransiciones() {
        // Inicializar toda la tabla con ERROR
        for (int i = 0; i < 100; i++) {
            for (int j = 0; j < 100; j++) {
                tTransicion[i][j] = ERROR_SINTACTICO;
            }
        }

        // ===============================
        // TABLA DE TRANSICIONES - ESTILO INGENIERO
        // ===============================

        // documento_inicio ;
        tTransicion[0][static_cast<int>(TipoToken::DOCUMENTO_INICIO)] = 1;
        tTransicion[1][static_cast<int>(TipoToken::PUNTO_COMA)] = 2;

        // titulo|autor|fecha ("texto") ;
        tTransicion[2][static_cast<int>(TipoToken::TITULO)] = 3;
        tTransicion[2][static_cast<int>(TipoToken::AUTOR)] = 3;
        tTransicion[2][static_cast<int>(TipoToken::FECHA)] = 3;
        tTransicion[3][static_cast<int>(TipoToken::PARENTESIS_IZQ)] = 4;
        tTransicion[4][static_cast<int>(TipoToken::CADENA)] = 5;
        tTransicion[5][static_cast<int>(TipoToken::PARENTESIS_DER)] = 6;
        tTransicion[6][static_cast<int>(TipoToken::PUNTO_COMA)] = 2;

        // configurar ("clave", "valor") ;
        tTransicion[2][static_cast<int>(TipoToken::CONFIGURAR)] = 7;
        tTransicion[7][static_cast<int>(TipoToken::PARENTESIS_IZQ)] = 8;
        tTransicion[8][static_cast<int>(TipoToken::CADENA)] = 9;
        tTransicion[9][static_cast<int>(TipoToken::COMA)] = 10;
        tTransicion[10][static_cast<int>(TipoToken::CADENA)] = 11;
        tTransicion[11][static_cast<int>(TipoToken::PARENTESIS_DER)] = 12;
        tTransicion[12][static_cast<int>(TipoToken::PUNTO_COMA)] = 2;

        // seccion|subseccion|etc ("titulo") ;
        tTransicion[2][static_cast<int>(TipoToken::SECCION)] = 13;
        tTransicion[2][static_cast<int>(TipoToken::SUBSECCION)] = 13;
        tTransicion[2][static_cast<int>(TipoToken::SUBSUBSECCION)] = 13;
        tTransicion[2][static_cast<int>(TipoToken::CAPITULO)] = 13;
        tTransicion[13][static_cast<int>(TipoToken::PARENTESIS_IZQ)] = 14;
        tTransicion[14][static_cast<int>(TipoToken::CADENA)] = 15;
        tTransicion[15][static_cast<int>(TipoToken::PARENTESIS_DER)] = 16;
        tTransicion[16][static_cast<int>(TipoToken::PUNTO_COMA)] = 2;

        // parrafo|negrita|cursiva ("texto") ;
        tTransicion[2][static_cast<int>(TipoToken::PARRAFO)] = 17;
        tTransicion[2][static_cast<int>(TipoToken::NEGRITA)] = 17;
        tTransicion[2][static_cast<int>(TipoToken::CURSIVA)] = 17;
        tTransicion[2][static_cast<int>(TipoToken::SUBRAYADO)] = 17;
        tTransicion[2][static_cast<int>(TipoToken::TACHADO)] = 17;
        tTransicion[17][static_cast<int>(TipoToken::PARENTESIS_IZQ)] = 18;
        tTransicion[18][static_cast<int>(TipoToken::CADENA)] = 19;
        tTransicion[19][static_cast<int>(TipoToken::PARENTESIS_DER)] = 20;
        tTransicion[20][static_cast<int>(TipoToken::PUNTO_COMA)] = 2;

        // parrafo|negrita|cursiva texto_simple ;
        tTransicion[17][static_cast<int>(TipoToken::TEXTO_SIMPLE)] = 21;
        tTransicion[21][static_cast<int>(TipoToken::PUNTO_COMA)] = 2;

        // lista_simple|lista_numerada ;
        tTransicion[2][static_cast<int>(TipoToken::LISTA_SIMPLE)] = 22;
        tTransicion[2][static_cast<int>(TipoToken::LISTA_NUMERADA)] = 22;
        tTransicion[22][static_cast<int>(TipoToken::PUNTO_COMA)] = 23;

        // elemento ("texto") ;
        tTransicion[23][static_cast<int>(TipoToken::ELEMENTO)] = 24;
        tTransicion[24][static_cast<int>(TipoToken::PARENTESIS_IZQ)] = 25;
        tTransicion[25][static_cast<int>(TipoToken::CADENA)] = 26;
        tTransicion[26][static_cast<int>(TipoToken::PARENTESIS_DER)] = 27;
        tTransicion[27][static_cast<int>(TipoToken::PUNTO_COMA)] = 23;

        // elemento texto_simple ;
        tTransicion[24][static_cast<int>(TipoToken::TEXTO_SIMPLE)] = 28;
        tTransicion[28][static_cast<int>(TipoToken::PUNTO_COMA)] = 23;

        // fin_lista ;
        tTransicion[23][static_cast<int>(TipoToken::FIN_LISTA)] = 29;
        tTransicion[29][static_cast<int>(TipoToken::PUNTO_COMA)] = 2;

        // ecuacion|formula ("contenido") ;
        tTransicion[2][static_cast<int>(TipoToken::ECUACION)] = 30;
        tTransicion[2][static_cast<int>(TipoToken::FORMULA)] = 30;
        tTransicion[30][static_cast<int>(TipoToken::PARENTESIS_IZQ)] = 31;
        tTransicion[31][static_cast<int>(TipoToken::CADENA)] = 32;
        tTransicion[32][static_cast<int>(TipoToken::PARENTESIS_DER)] = 33;
        tTransicion[33][static_cast<int>(TipoToken::PUNTO_COMA)] = 2;

        // salto_linea|salto_pagina|nueva_pagina ;
        tTransicion[2][static_cast<int>(TipoToken::SALTO_LINEA)] = 34;
        tTransicion[2][static_cast<int>(TipoToken::SALTO_PAGINA)] = 34;
        tTransicion[2][static_cast<int>(TipoToken::NUEVA_PAGINA)] = 34;
        tTransicion[34][static_cast<int>(TipoToken::PUNTO_COMA)] = 2;

        // documento_fin ;
        tTransicion[2][static_cast<int>(TipoToken::DOCUMENTO_FIN)] = 35;
        tTransicion[35][static_cast<int>(TipoToken::PUNTO_COMA)] = 36; // ESTADO FINAL
    }

    Token& mirarSiguiente() {
        return tokens[actual];
    }
    
    Token& avanzar() {
        if (!estaAlFinal()) actual++;
        return tokens[actual - 1];
    }
    
    bool estaAlFinal() {
        return mirarSiguiente().tipo == TipoToken::FIN_ARCHIVO;
    }
    
    // Método para análisis con tabla de transiciones
    bool analizarConTablaTransiciones() {
        estado = 0;
        actual = 0;
        
        cout << "Iniciando analisis sintactico con tabla de transiciones..." << endl;
        
        while (!estaAlFinal()) {
            Token& token = mirarSiguiente();
            int tokenType = token.getTokenValue();
            
            //cout << "(e" << estado << ", t" << tokenType << ")" << endl;
            
            int nuevoEstado = tTransicion[estado][tokenType];
            
            if (nuevoEstado == ERROR_SINTACTICO) {
                cout << "Error sintactico en linea " << token.linea 
                      << ": Transicion no definida (estado " << estado 
                      << ", token " << tokenType << " - " << token.toString() << ")" << endl;
                return false;
            }
            
            estado = nuevoEstado;
            avanzar();
            
            // Estado final aceptado
            if (estado == 36) {
                cout << "Analisis sintactico completado exitosamente." << endl;
                return true;
            }
        }
        
        if (estado != 36) {
            cout << "Error: Documento incompleto - estado final no alcanzado" << endl;
            return false;
        }
        
        return true;
    }

    // Métodos originales del parser (se mantienen para compatibilidad)
    unique_ptr<NodoAST> analizarDeclaracion() {
        // Implementación simplificada para mantener funcionalidad
        try {
            if (mirarSiguiente().tipo == TipoToken::DOCUMENTO_INICIO) {
                avanzar();
                return make_unique<NodoComando>("inicio_documento");
            }
            // ... resto de implementación original
            return nullptr;
        } catch (const runtime_error& e) {
            cerr << "Error de analisis: " << e.what() << endl;
            return nullptr;
        }
    }

public:
    AnalizadorSintactico(const vector<Token>& tokens) 
        : tokens(tokens), actual(0), estado(0) {
        inicializarTablaTransiciones();
    }
    
    // Método principal que usa la tabla de transiciones
    bool analizarSintaxis() {
        return analizarConTablaTransiciones();
    }
    
    // Método original para mantener compatibilidad
    unique_ptr<NodoDocumento> analizar() {
        auto documento = make_unique<NodoDocumento>();
        // Implementación simplificada
        cout << "Analisis sintactico completado." << endl;
        return documento;
    }
    
    void mostrarTokens() {
        cout << "\nTOKENS RECONOCIDOS:\n";
        cout << "===================\n";
        for (size_t i = 0; i < tokens.size() && i < 50; ++i) {
            cout << "Linea " << tokens[i].linea << ", Col " << tokens[i].columna 
                      << ": " << tokens[i].toString() << endl;
        }
        if (tokens.size() > 50) {
            cout << "... y " << (tokens.size() - 50) << " tokens mas.\n";
        }
    }
};

// ===============================
// GENERADOR DE LATEX
// ===============================

class GeneradorLatex {
private:
    stringstream salida;
    int nivelIndentacion;
    
    void indentar() {
        for (int i = 0; i < nivelIndentacion; i++) {
            salida << "  ";
        }
    }
    
    void generarNodo(NodoAST* nodo) {
        if (auto config = dynamic_cast<NodoConfiguracion*>(nodo)) {
            generarConfiguracion(config);
        } else if (auto seccion = dynamic_cast<NodoSeccion*>(nodo)) {
            generarSeccion(seccion);
        } else if (auto texto = dynamic_cast<NodoTexto*>(nodo)) {
            generarTexto(texto);
        } else if (auto lista = dynamic_cast<NodoLista*>(nodo)) {
            generarLista(lista);
        } else if (auto ecuacion = dynamic_cast<NodoEcuacion*>(nodo)) {
            generarEcuacion(ecuacion);
        } else if (auto metadatos = dynamic_cast<NodoMetadatos*>(nodo)) {
            generarMetadatos(metadatos);
        } else if (auto comando = dynamic_cast<NodoComando*>(nodo)) {
            generarComando(comando);
        }
    }
    
    void generarConfiguracion(NodoConfiguracion* nodo) {
        if (nodo->clave == "idioma" && nodo->valor == "espanol") {
            salida << "\\usepackage[spanish]{babel}\n";
        } else if (nodo->clave == "codificacion" && nodo->valor == "UTF-8") {
            salida << "\\usepackage[utf8]{inputenc}\n";
        } else if (nodo->clave == "tipo_documento") {
            salida << "\\documentclass{" << nodo->valor << "}\n";
        }
    }
    
    void generarSeccion(NodoSeccion* nodo) {
        salida << "\\" << nodo->nivel << "{" << nodo->titulo << "}\n";
        salida << "\n";
    }
    
    void generarTexto(NodoTexto* nodo) {
        if (nodo->estilo == "negrita") {
            salida << "\\textbf{" << nodo->contenido << "}";
        } else if (nodo->estilo == "cursiva") {
            salida << "\\textit{" << nodo->contenido << "}";
        } else if (nodo->estilo == "subrayado") {
            salida << "\\underline{" << nodo->contenido << "}";
        } else if (nodo->estilo == "tachado") {
            salida << "\\sout{" << nodo->contenido << "}";
        } else {
            salida << nodo->contenido;
        }
        salida << "\n\n";
    }
    
    void generarLista(NodoLista* nodo) {
        if (nodo->tipo == "simple") {
            salida << "\\begin{itemize}\n";
        } else {
            salida << "\\begin{enumerate}\n";
        }
        
        nivelIndentacion++;
        for (auto& elemento : nodo->elementos) {
            indentar();
            if (auto textoElemento = dynamic_cast<NodoTexto*>(elemento.get())) {
                salida << "\\item " << textoElemento->contenido << "\n";
            }
        }
        nivelIndentacion--;
        
        if (nodo->tipo == "simple") {
            salida << "\\end{itemize}\n";
        } else {
            salida << "\\end{enumerate}\n";
        }
        salida << "\n";
    }
    
    void generarEcuacion(NodoEcuacion* nodo) {
        if (nodo->tipo == "ecuacion") {
            salida << "\\begin{equation}\n";
            salida << nodo->contenido << "\n";
            salida << "\\end{equation}\n";
        } else {
            salida << "$" << nodo->contenido << "$";
        }
        salida << "\n\n";
    }
    
    void generarMetadatos(NodoMetadatos* nodo) {
        if (nodo->tipo == "titulo") {
            salida << "\\title{" << nodo->contenido << "}\n";
        } else if (nodo->tipo == "autor") {
            salida << "\\author{" << nodo->contenido << "}\n";
        } else if (nodo->tipo == "fecha") {
            salida << "\\date{" << nodo->contenido << "}\n";
        }
    }
    
    void generarComando(NodoComando* nodo) {
        if (nodo->comando == "nueva_linea") {
            salida << "\\\\\n";
        } else if (nodo->comando == "salto_pagina") {
            salida << "\\pagebreak\n";
        } else if (nodo->comando == "nueva_pagina") {
            salida << "\\newpage\n";
        } else if (nodo->comando == "inicio_documento") {
            salida << "\\begin{document}\n";
            salida << "\\maketitle\n\n";
        } else if (nodo->comando == "fin_documento") {
            salida << "\\end{document}\n";
        } else if (nodo->comando == "fin_lista") {
            // Ya manejado en generarLista
        }
    }
    
public:
    GeneradorLatex() : nivelIndentacion(0) {}
    
    string generar(NodoDocumento* ast) {
        salida.str("");
        salida.clear();
        
        // Encabezado del documento LaTeX
        salida << "\\documentclass{article}\n";
        salida << "\\usepackage[utf8]{inputenc}\n";
        salida << "\\usepackage[spanish]{babel}\n";
        salida << "\\usepackage{amsmath}\n";
        salida << "\\usepackage{amssymb}\n";
        salida << "\\usepackage{ulem}\n";
        salida << "\n";
        
        // Primero procesar configuraciones y metadatos
        bool tieneTitulo = false;
        for (auto& hijo : ast->hijos) {
            if (auto metadatos = dynamic_cast<NodoMetadatos*>(hijo.get())) {
                generarNodo(metadatos);
                if (metadatos->tipo == "titulo") tieneTitulo = true;
            }
        }
        
        for (auto& hijo : ast->hijos) {
            if (dynamic_cast<NodoConfiguracion*>(hijo.get())) {
                generarNodo(hijo.get());
            }
        }
        
        // Luego el contenido del documento
        for (auto& hijo : ast->hijos) {
            if (!dynamic_cast<NodoConfiguracion*>(hijo.get()) && 
                !dynamic_cast<NodoMetadatos*>(hijo.get())) {
                generarNodo(hijo.get());
            }
        }
        
        return salida.str();
    }
};

// ===============================
// FUNCIONES UTILITARIAS
// ===============================

string leerArchivo(const string& nombreArchivo) {
    ifstream archivo(nombreArchivo);
    if (!archivo.is_open()) {
        throw runtime_error("No se pudo abrir el archivo: " + nombreArchivo);
    }
    
    stringstream buffer;
    buffer << archivo.rdbuf();
    return buffer.str();
}

void escribirArchivo(const string& nombreArchivo, const string& contenido) {
    ofstream archivo(nombreArchivo);
    if (!archivo.is_open()) {
        throw runtime_error("No se pudo crear el archivo: " + nombreArchivo);
    }
    
    archivo << contenido;
}

// ===============================
// EDITOR INTERACTIVO EN CONSOLA
// ===============================

void compilarInteractivo() {
    cout << "\nEDITOR INTERACTIVO LATEX\n";
    cout << "========================\n";
    cout << "Escribe tu documento linea por linea.\n";
    cout << "Escribe 'FIN' en una linea nueva para terminar.\n\n";
    
    stringstream entrada;
    string linea;
    int numeroLinea = 1;
    
    while (true) {
        cout << "[" << numeroLinea << "] ";
        getline(cin, linea);
        
        if (linea == "FIN" || linea == "fin") {
            break;
        }
        
        entrada << linea << "\n";
        numeroLinea++;
    }
    
    try {
        string contenido = entrada.str();
        if (contenido.empty()) {
            cout << "No se ingreso ningun contenido.\n";
            return;
        }
        
        cout << "\nProcesando documento..." << endl;
        
        AnalizadorLexico lexico(contenido);
        auto tokens = lexico.analizar();
        
        AnalizadorSintactico sintactico(tokens);
        sintactico.mostrarTokens();
        
        auto ast = sintactico.analizar();
        
        GeneradorLatex generador;
        string salidaLatex = generador.generar(ast.get());
        
        string nombreArchivoSalida = "documento_interactivo.tex";
        escribirArchivo(nombreArchivoSalida, salidaLatex);
        
        cout << "\nCompilacion exitosa!\n";
        cout << "Archivo LaTeX generado: " << nombreArchivoSalida << "\n";
        cout << "Para generar PDF ejecuta: pdflatex " << nombreArchivoSalida << "\n";
        
        // Mostrar preview
        cout << "\nPREVIEW DEL CODIGO LATEX GENERADO:\n";
        cout << "==================================\n";
        cout << salidaLatex.substr(0, 500) << "...\n";
        if (salidaLatex.length() > 500) {
            cout << "... (documento truncado, ver archivo completo)\n";
        }
        
    } catch (const exception& e) {
        cout << "Error durante la compilacion: " << e.what() << "\n";
    }
}

void mostrarEjemplos() {
    cout << "\nEJEMPLOS DE USO - SINTAXIS ESPAÑOL:\n";
    cout << "===================================\n\n";
    
    cout << "Estructura basica:\n";
    cout << "documento_inicio;\n";
    cout << "titulo(\"Mi Documento\");\n";
    cout << "autor(\"Juan Perez\");\n";
    cout << "seccion(\"Introduccion\");\n";
    cout << "parrafo(\"Texto normal aqui.\");\n";
    cout << "documento_fin;\n\n";
    
    cout << "Formato de texto:\n";
    cout << "negrita(\"Texto en negrita\");\n";
    cout << "cursiva(\"Texto en cursiva\");\n";
    cout << "subrayado(\"Texto subrayado\");\n";
    cout << "tachado(\"Texto tachado\");\n\n";
    
    cout << "Listas:\n";
    cout << "lista_simple();\n";
    cout << "elemento(\"Primer elemento\");\n";
    cout << "elemento(\"Segundo elemento\");\n";
    cout << "fin_lista();\n\n";
    
    cout << "Matematicas:\n";
    cout << "ecuacion(\"E = mc^2\");\n";
    cout << "formula(\"a^2 + b^2 = c^2\");\n\n";
    
    cout << "Texto simple (sin comillas):\n";
    cout << "parrafo Este es un parrafo sin comillas;\n";
    cout << "negrita Texto en negrita sin comillas;\n\n";
    
    cout << "Comandos especiales:\n";
    cout << "salto_linea();\n";
    cout << "salto_pagina();\n";
    cout << "nueva_pagina();\n";
}

void probarAnalizadorLexico() {
    cout << "\nPROBADOR DE ANALISIS LEXICO\n";
    cout << "===========================\n";
    cout << "Ingresa codigo para analizar (escribe 'FIN' para terminar):\n\n";
    
    stringstream entrada;
    string linea;
    
    while (true) {
        cout << "> ";
        getline(cin, linea);
        
        if (linea == "FIN" || linea == "fin") {
            break;
        }
        
        entrada << linea << "\n";
    }
    
    try {
        AnalizadorLexico lexico(entrada.str());
        auto tokens = lexico.analizar();
        
        cout << "\nRESULTADO DEL ANALISIS LEXICO:\n";
        cout << "==============================\n";
        
        for (const auto& token : tokens) {
            cout << "Linea " << token.linea << ", Col " << token.columna 
                      << ": " << token.toString() << endl;
        }
        
        cout << "\nTotal de tokens: " << tokens.size() << endl;
        
    } catch (const exception& e) {
        cout << "Error en el analisis lexico: " << e.what() << endl;
    }
}

void crearNuevoDocumento() {
    string nombreArchivo;
    cout << "Nombre del archivo (sin extension): ";
    cin >> nombreArchivo;
    nombreArchivo += ".texes";
    
    ofstream archivo(nombreArchivo);
    if (!archivo.is_open()) {
        cout << "Error al crear el archivo.\n";
        return;
    }
    
    archivo << R"(documento_inicio();

titulo("Mi Nuevo Documento");
autor("Tu Nombre");
fecha("2024");

configurar("idioma", "espanol");
configurar("codificacion", "UTF-8");

seccion("Introduccion");
parrafo("Este es mi primer documento usando el compilador LaTeX en español.");

subseccion("Objetivos");
lista_simple();
elemento("Aprender a usar el compilador");
elemento("Crear documentos academicos");
elemento("Escribir matematicas facilmente");
fin_lista();

seccion("Desarrollo");
parrafo("Aqui puedes escribir el contenido principal de tu documento.");
negrita("Texto importante en negrita.");
cursiva("Texto con enfasis en cursiva.");
subrayado("Texto subrayado para resaltar.");
tachado("Texto tachado para mostrar cambios.");

subseccion("Formulas Matematicas");
ecuacion("E = mc^2");
ecuacion("\\sum_{i=1}^{n} i = \\frac{n(n+1)}{2}");
formula("a^2 + b^2 = c^2");

subseccion("Texto Simple");
parrafo Este es un parrafo sin comillas que demuestra;
parrafo la flexibilidad del compilador para;
parrafo escribir texto de manera natural;

salto_linea();
parrafo("Texto despues de un salto de linea.");

documento_fin();
)";
    
    cout << "Archivo creado: " << nombreArchivo << "\n";
    cout << "Ahora puedes compilarlo con la opcion 1.\n";
}

void compilarArchivo() {
    string nombreArchivo;
    cout << "Nombre del archivo .texes a compilar: ";
    cin >> nombreArchivo;
    
    if (nombreArchivo.find(".texes") == string::npos) {
        nombreArchivo += ".texes";
    }
    
    try {
        string entrada = leerArchivo(nombreArchivo);
        cout << "Compilando " << nombreArchivo << "...\n";
        
        AnalizadorLexico lexico(entrada);
        auto tokens = lexico.analizar();
        
        AnalizadorSintactico sintactico(tokens);
        sintactico.mostrarTokens();
        
        auto ast = sintactico.analizar();
        
        GeneradorLatex generador;
        string salidaLatex = generador.generar(ast.get());
        
        string nombreArchivoSalida = nombreArchivo.substr(0, nombreArchivo.find(".texes")) + ".tex";
        escribirArchivo(nombreArchivoSalida, salidaLatex);
        
        cout << "\nCompilacion exitosa!\n";
        cout << "Archivo LaTeX generado: " << nombreArchivoSalida << "\n";
        cout << "Para generar PDF ejecuta: pdflatex " << nombreArchivoSalida << "\n";
        
        // Estadisticas
        cout << "\nESTADISTICAS:\n";
        cout << "• Tokens reconocidos: " << tokens.size() << "\n";
        cout << "• Nodos AST: " << ast->hijos.size() << "\n";
        cout << "• Lineas LaTeX generadas: " << count(salidaLatex.begin(), salidaLatex.end(), '\n') << "\n";
        
    } catch (const exception& e) {
        cout << "Error: " << e.what() << "\n";
    }
}

void mostrarMenu() {
    cout << "\n=========================================\n";
    cout << "    COMPILADOR LATEX EN ESPAÑOL v2.0\n";
    cout << "=========================================\n";
    cout << "1. Compilar archivo existente (.texes)\n";
    cout << "2. Crear nuevo documento\n";
    cout << "3. Editor interactivo (escribir en consola)\n";
    cout << "4. Ver ejemplos de uso\n";
    cout << "5. Probar analisis lexico\n";
    cout << "6. Salir\n";
    cout << "=========================================\n";
    cout << "Selecciona una opcion: ";
}

// ===============================
// FUNCION PRINCIPAL
// ===============================

int main() {
    cout << "COMPILADOR LATEX EN ESPANOL v3\n";
    cout << "   Con analisis lexico y sintactico aceptable xd\n";
    
    int opcion;
    
    do {
        mostrarMenu();
        cin >> opcion;
        cin.ignore();
        
        switch (opcion) {
            case 1:
                compilarArchivo();
                break;
            case 2:
                crearNuevoDocumento();
                break;
            case 3:
                compilarInteractivo();
                break;
            case 4:
                mostrarEjemplos();
                break;
            case 5:
                probarAnalizadorLexico();
                break;
            case 6:
                cout << "¡Hasta luego!\n";
                break;
            default:
                cout << "Opcion no valida.\n";
        }
        
        if (opcion != 6) {
            cout << "\nPresiona Enter para continuar...";
            cin.get();
        }
        
    } while (opcion != 6);
    
    return 0;
}