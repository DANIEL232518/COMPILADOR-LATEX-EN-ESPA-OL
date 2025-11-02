// Compilador LaTeX en Español - Versión JavaScript
class LatexCompiler {
    constructor() {
        this.tokens = [];
        this.current = 0;
    }

    // Análisis léxico
    lex(input) {
        const tokens = [];
        let line = 1;
        let pos = 0;

        const keywords = {
            'documento_inicio': 'DOCUMENTO_INICIO',
            'documento_fin': 'DOCUMENTO_FIN',
            'configurar': 'CONFIGURAR',
            'seccion': 'SECCION',
            'subseccion': 'SUBSECCION',
            'subsubseccion': 'SUBSUBSECCION',
            'capitulo': 'CAPITULO',
            'negrita': 'NEGRITA',
            'cursiva': 'CURSIVA',
            'subrayado': 'SUBRAYADO',
            'tachado': 'TACHADO',
            'parrafo': 'PARRAFO',
            'lista_simple': 'LISTA_SIMPLE',
            'lista_numerada': 'LISTA_NUMERADA',
            'elemento': 'ELEMENTO',
            'fin_lista': 'FIN_LISTA',
            'ecuacion': 'ECUACION',
            'formula': 'FORMULA',
            'titulo': 'TITULO',
            'autor': 'AUTOR',
            'fecha': 'FECHA',
            'salto_linea': 'SALTO_LINEA',
            'salto_pagina': 'SALTO_PAGINA',
            'nueva_pagina': 'NUEVA_PAGINA'
        };

        const symbols = {
            '(': 'PARENTESIS_IZQ',
            ')': 'PARENTESIS_DER',
            '{': 'LLAVE_IZQ',
            '}': 'LLAVE_DER',
            '[': 'CORCHETE_IZQ',
            ']': 'CORCHETE_DER',
            ',': 'COMA',
            ';': 'PUNTO_COMA',
            '=': 'IGUAL',
            '"': 'COMILLA'
        };

        while (pos < input.length) {
            let char = input[pos];

            // Saltar espacios y tabs
            if (char === ' ' || char === '\t') {
                pos++;
                continue;
            }

            // Saltar comentarios
            if (char === '%') {
                while (pos < input.length && input[pos] !== '\n') {
                    pos++;
                }
                continue;
            }

            // Saltar nuevas líneas
            if (char === '\n') {
                line++;
                pos++;
                continue;
            }

            // Identificadores y palabras clave
            if (/[a-zA-Z_]/.test(char)) {
                let identifier = '';
                while (pos < input.length && /[a-zA-Z0-9_]/.test(input[pos])) {
                    identifier += input[pos];
                    pos++;
                }

                if (keywords[identifier]) {
                    tokens.push({ type: keywords[identifier], value: identifier, line });
                } else {
                    tokens.push({ type: 'IDENTIFICADOR', value: identifier, line });
                }
                continue;
            }

            // Números
            if (/[0-9]/.test(char)) {
                let number = '';
                while (pos < input.length && /[0-9.]/.test(input[pos])) {
                    number += input[pos];
                    pos++;
                }
                tokens.push({ type: 'NUMERO', value: number, line });
                continue;
            }

            // Cadenas de texto
            if (char === '"') {
                let string = '';
                pos++; // Saltar comilla inicial
                
                while (pos < input.length && input[pos] !== '"') {
                    if (input[pos] === '\\') {
                        pos++; // Carácter de escape
                        if (pos < input.length) {
                            switch (input[pos]) {
                                case 'n': string += '\n'; break;
                                case 't': string += '\t'; break;
                                case '"': string += '"'; break;
                                case '\\': string += '\\'; break;
                                default: string += '\\' + input[pos]; break;
                            }
                        }
                    } else {
                        string += input[pos];
                    }
                    pos++;
                }
                
                if (pos < input.length && input[pos] === '"') {
                    pos++; // Saltar comilla final
                }
                
                tokens.push({ type: 'CADENA', value: string, line });
                continue;
            }

            // Símbolos
            if (symbols[char]) {
                tokens.push({ type: symbols[char], value: char, line });
                pos++;
                continue;
            }

            // Texto simple (sin comillas)
            if (/[^\s;]/.test(char)) {
                let text = '';
                while (pos < input.length && input[pos] !== ';' && input[pos] !== '\n') {
                    text += input[pos];
                    pos++;
                }
                text = text.trim();
                if (text) {
                    tokens.push({ type: 'TEXTO_SIMPLE', value: text, line });
                }
                continue;
            }

            pos++;
        }

        tokens.push({ type: 'FIN_ARCHIVO', value: '', line });
        return tokens;
    }

    // Análisis sintáctico y generación de LaTeX
    parse(tokens) {
        this.tokens = tokens;
        this.current = 0;
        
        let latex = '\\documentclass{article}\n';
        latex += '\\usepackage[utf8]{inputenc}\n';
        latex += '\\usepackage[spanish]{babel}\n';
        latex += '\\usepackage{amsmath}\n';
        latex += '\\usepackage{amssymb}\n';
        latex += '\\usepackage{ulem}\n\n';

        const metadata = {
            titulo: 'Documento Sin Título',
            autor: '',
            fecha: '\\today'
        };

        let inList = false;
        let listType = '';

        while (!this.isAtEnd()) {
            const token = this.advance();

            switch (token.type) {
                case 'DOCUMENTO_INICIO':
                    this.consume('PUNTO_COMA', 'Se esperaba ; después de documento_inicio');
                    break;

                case 'TITULO':
                    this.consume('PARENTESIS_IZQ', 'Se esperaba ( después de titulo');
                    const titulo = this.consume('CADENA', 'Se esperaba cadena para título').value;
                    this.consume('PARENTESIS_DER', 'Se esperaba ) después del título');
                    this.consume('PUNTO_COMA', 'Se esperaba ;');
                    metadata.titulo = titulo;
                    latex += `\\title{${titulo}}\n`;
                    break;

                case 'AUTOR':
                    this.consume('PARENTESIS_IZQ', 'Se esperaba ( después de autor');
                    const autor = this.consume('CADENA', 'Se esperaba cadena para autor').value;
                    this.consume('PARENTESIS_DER', 'Se esperaba ) después del autor');
                    this.consume('PUNTO_COMA', 'Se esperaba ;');
                    metadata.autor = autor;
                    latex += `\\author{${autor}}\n`;
                    break;

                case 'FECHA':
                    this.consume('PARENTESIS_IZQ', 'Se esperaba ( después de fecha');
                    const fecha = this.consume('CADENA', 'Se esperaba cadena para fecha').value;
                    this.consume('PARENTESIS_DER', 'Se esperaba ) después de la fecha');
                    this.consume('PUNTO_COMA', 'Se esperaba ;');
                    metadata.fecha = fecha;
                    latex += `\\date{${fecha}}\n`;
                    break;

                case 'CONFIGURAR':
                    this.consume('PARENTESIS_IZQ', 'Se esperaba ( después de configurar');
                    const clave = this.consume('CADENA', 'Se esperaba cadena para clave').value;
                    this.consume('COMA', 'Se esperaba ,');
                    const valor = this.consume('CADENA', 'Se esperaba cadena para valor').value;
                    this.consume('PARENTESIS_DER', 'Se esperaba )');
                    this.consume('PUNTO_COMA', 'Se esperaba ;');
                    
                    if (clave === 'idioma' && valor === 'espanol') {
                        latex += '\\usepackage[spanish]{babel}\n';
                    } else if (clave === 'codificacion' && valor === 'UTF-8') {
                        latex += '\\usepackage[utf8]{inputenc}\n';
                    }
                    break;

                case 'SECCION':
                case 'SUBSECCION':
                case 'SUBSUBSECCION':
                case 'CAPITULO':
                    if (inList) {
                        latex += `\\end{${listType === 'LISTA_SIMPLE' ? 'itemize' : 'enumerate'}}\n\n`;
                        inList = false;
                    }
                    
                    const nivel = token.type.toLowerCase();
                    this.consume('PARENTESIS_IZQ', `Se esperaba ( después de ${nivel}`);
                    const tituloSeccion = this.consume('CADENA', 'Se esperaba cadena para el título').value;
                    this.consume('PARENTESIS_DER', 'Se esperaba )');
                    this.consume('PUNTO_COMA', 'Se esperaba ;');
                    
                    let latexCommand = nivel;
                    if (nivel === 'subsubseccion') latexCommand = 'subsubsection';
                    if (nivel === 'capitulo') latexCommand = 'chapter';
                    
                    latex += `\\${latexCommand}{${tituloSeccion}}\n\n`;
                    break;

                case 'LISTA_SIMPLE':
                case 'LISTA_NUMERADA':
                    this.consume('PUNTO_COMA', 'Se esperaba ;');
                    inList = true;
                    listType = token.type;
                    latex += `\\begin{${token.type === 'LISTA_SIMPLE' ? 'itemize' : 'enumerate'}}\n`;
                    break;

                case 'ELEMENTO':
                    let elementoTexto = '';
                    if (this.check('PARENTESIS_IZQ')) {
                        this.advance(); // (
                        elementoTexto = this.consume('CADENA', 'Se esperaba cadena para elemento').value;
                        this.advance(); // )
                    } else if (this.check('TEXTO_SIMPLE')) {
                        elementoTexto = this.advance().value;
                    }
                    this.consume('PUNTO_COMA', 'Se esperaba ;');
                    
                    if (inList) {
                        latex += `  \\item ${elementoTexto}\n`;
                    }
                    break;

                case 'FIN_LISTA':
                    this.consume('PUNTO_COMA', 'Se esperaba ;');
                    if (inList) {
                        latex += `\\end{${listType === 'LISTA_SIMPLE' ? 'itemize' : 'enumerate'}}\n\n`;
                        inList = false;
                    }
                    break;

                case 'PARRAFO':
                case 'NEGRITA':
                case 'CURSIVA':
                case 'SUBRAYADO':
                case 'TACHADO':
                    let texto = '';
                    if (this.check('PARENTESIS_IZQ')) {
                        this.advance(); // (
                        texto = this.consume('CADENA', 'Se esperaba cadena para texto').value;
                        this.advance(); // )
                    } else if (this.check('TEXTO_SIMPLE')) {
                        texto = this.advance().value;
                    }
                    this.consume('PUNTO_COMA', 'Se esperaba ;');

                    if (inList) {
                        let itemText = texto;
                        if (token.type === 'NEGRITA') itemText = `\\textbf{${texto}}`;
                        else if (token.type === 'CURSIVA') itemText = `\\textit{${texto}}`;
                        else if (token.type === 'SUBRAYADO') itemText = `\\underline{${texto}}`;
                        else if (token.type === 'TACHADO') itemText = `\\sout{${texto}}`;
                        
                        latex += `  \\item ${itemText}\n`;
                    } else {
                        if (token.type === 'NEGRITA') latex += `\\textbf{${texto}}\n\n`;
                        else if (token.type === 'CURSIVA') latex += `\\textit{${texto}}\n\n`;
                        else if (token.type === 'SUBRAYADO') latex += `\\underline{${texto}}\n\n`;
                        else if (token.type === 'TACHADO') latex += `\\sout{${texto}}\n\n`;
                        else latex += `${texto}\n\n`;
                    }
                    break;

                case 'ECUACION':
                case 'FORMULA':
                    this.consume('PARENTESIS_IZQ', 'Se esperaba ( después de ecuación/fórmula');
                    const ecuacion = this.consume('CADENA', 'Se esperaba cadena para ecuación').value;
                    this.consume('PARENTESIS_DER', 'Se esperaba )');
                    this.consume('PUNTO_COMA', 'Se esperaba ;');
                    
                    if (token.type === 'ECUACION') {
                        latex += `\\begin{equation}\n${ecuacion}\n\\end{equation}\n\n`;
                    } else {
                        latex += `$${ecuacion}$\n\n`;
                    }
                    break;

                case 'SALTO_LINEA':
                    this.consume('PUNTO_COMA', 'Se esperaba ;');
                    latex += '\\\\\n';
                    break;

                case 'SALTO_PAGINA':
                    this.consume('PUNTO_COMA', 'Se esperaba ;');
                    latex += '\\pagebreak\n';
                    break;

                case 'NUEVA_PAGINA':
                    this.consume('PUNTO_COMA', 'Se esperaba ;');
                    latex += '\\newpage\n';
                    break;

                case 'DOCUMENTO_FIN':
                    this.consume('PUNTO_COMA', 'Se esperaba ;');
                    if (inList) {
                        latex += `\\end{${listType === 'LISTA_SIMPLE' ? 'itemize' : 'enumerate'}}\n\n`;
                    }
                    break;
            }
        }

        // Agregar encabezado del documento LaTeX
        let finalLatex = latex;
        finalLatex += '\\begin{document}\n';
        finalLatex += '\\maketitle\n\n';
        finalLatex += latex.includes('\\section') ? '' : '\\section{Contenido}\n\n';
        finalLatex += '\\end{document}';

        return finalLatex;
    }

    consume(expectedType, errorMessage) {
        if (this.check(expectedType)) {
            return this.advance();
        }
        throw new Error(`Error sintáctico: ${errorMessage}`);
    }

    check(expectedType) {
        if (this.isAtEnd()) return false;
        return this.peek().type === expectedType;
    }

    peek() {
        return this.tokens[this.current];
    }

    advance() {
        if (!this.isAtEnd()) this.current++;
        return this.tokens[this.current - 1];
    }

    isAtEnd() {
        return this.peek().type === 'FIN_ARCHIVO';
    }

    compile(input) {
        try {
            const tokens = this.lex(input);
            const latex = this.parse(tokens);
            return { success: true, latex: latex, tokens: tokens };
        } catch (error) {
            return { success: false, error: error.message, tokens: [] };
        }
    }
}

// Instancia global del compilador
const compiler = new LatexCompiler();

// Funciones de la interfaz
function compileLatex() {
    const input = document.getElementById('inputCode').value;
    const status = document.getElementById('status');
    const output = document.getElementById('output');

    status.textContent = '🔄 Compilando...';
    status.className = 'status';

    setTimeout(() => {
        try {
            const result = compiler.compile(input);
            
            if (result.success) {
                output.textContent = result.latex;
                status.textContent = '✅ Compilación exitosa!';
                status.className = 'status success';
            } else {
                output.textContent = `Error: ${result.error}`;
                status.textContent = '❌ Error de compilación';
                status.className = 'status error';
            }
        } catch (error) {
            output.textContent = `Error fatal: ${error.message}`;
            status.textContent = '💥 Error fatal';
            status.className = 'status error';
        }
    }, 500);
}

function loadExample() {
    const examples = [
        `documento_inicio();

titulo("Mi Documento de Ejemplo");
autor("Juan Pérez");
fecha("2024");

seccion("Introducción");
parrafo("Este es un párrafo de ejemplo que demuestra las capacidades del compilador.");

subseccion("Características");
lista_simple();
elemento("Fácil de usar");
elemento("Sintaxis en español");
elemento("Genera código LaTeX válido");
fin_lista();

documento_fin();`
    ];

    const randomExample = examples[Math.floor(Math.random() * examples.length)];
    document.getElementById('inputCode').value = randomExample;
    
    const status = document.getElementById('status');
    status.textContent = '📚 Ejemplo cargado. Haz clic en "Compilar a LaTeX" para generar el código.';
    status.className = 'status';
}

function clearCode() {
    document.getElementById('inputCode').value = '';
    document.getElementById('output').textContent = '';
    
    const status = document.getElementById('status');
    status.textContent = '🗑️ Código limpiado. Listo para escribir nuevo código.';
    status.className = 'status';
}

function downloadLatex() {
    const latexCode = document.getElementById('output').textContent;
    if (!latexCode.trim()) {
        alert('Primero genera código LaTeX compilando tu documento.');
        return;
    }

    const blob = new Blob([latexCode], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'documento_generado.tex';
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
}

// Ejemplos específicos
function loadBasicExample() {
    document.getElementById('inputCode').value = `documento_inicio();

titulo("Documento Básico");
autor("Estudiante Ejemplar");
fecha("2024");

seccion("Primera Sección");
parrafo("Este es el primer párrafo de mi documento.");

subseccion("Subsección Ejemplo");
parrafo("Texto en una subsección.");

seccion("Conclusión");
parrafo("Este documento demuestra la estructura básica.");

documento_fin();`;

    document.getElementById('status').textContent = '📄 Ejemplo básico cargado. Compila para ver el resultado.';
}

function loadMathExample() {
    document.getElementById('inputCode').value = `documento_inicio();

titulo("Documento Matemático");
autor("Matemático");

seccion("Ecuaciones Importantes");
parrafo("Algunas ecuaciones famosas:");

ecuacion("E = mc^2");
formula("a^2 + b^2 = c^2");
ecuacion("\\\sum_{i=1}^{n} i = \\\frac{n(n+1)}{2}");

parrafo("Fórmula cuadrática:");
formula("x = \\\frac{-b \\\pm \\\sqrt{b^2 - 4ac}}{2a}");

documento_fin();`;

    document.getElementById('status').textContent = '∫ Ejemplo matemático cargado. Compila para ver las ecuaciones.';
}

function loadFormatExample() {
    document.getElementById('inputCode').value = `documento_inicio();

titulo("Formatos de Texto");

seccion("Estilos de Texto");
parrafo("Texto normal.");
negrita("Texto en negrita.");
cursiva("Texto en cursiva.");
subrayado("Texto subrayado.");
tachado("Texto tachado.");

parrafo("Texto " + negrita("combinado") + " con " + cursiva("diferentes") + " estilos.");

documento_fin();`;

    document.getElementById('status').textContent = '🎨 Ejemplo de formatos cargado. Compila para ver los estilos.';
}

function loadListExample() {
    document.getElementById('inputCode').value = `documento_inicio();

titulo("Ejemplo de Listas");

seccion("Lista Simple");
lista_simple();
elemento("Primer elemento");
elemento("Segundo elemento");
elemento("Tercer elemento");
fin_lista();

seccion("Lista Numerada");
lista_numerada();
elemento("Primer paso");
elemento("Segundo paso");
elemento("Tercer paso");
fin_lista();

documento_fin();`;

    document.getElementById('status').textContent = '📋 Ejemplo de listas cargado. Compila para ver las listas.';
}

// Compilar automáticamente al cargar la página con el ejemplo por defecto
window.addEventListener('load', function() {
    setTimeout(() => {
        compileLatex();
    }, 1000);
});