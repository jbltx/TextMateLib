// List of available grammars from textmate-grammars-themes
// This is a curated list of popular languages

const BASE_PATH = '/tm-grammars';

export async function getAvailableGrammars() {
    return [
        {
            name: 'javascript',
            displayName: 'JavaScript',
            scopeName: 'source.js',
            path: `${BASE_PATH}/javascript.json`,
        },
        {
            name: 'typescript',
            displayName: 'TypeScript',
            scopeName: 'source.ts',
            path: `${BASE_PATH}/typescript.json`,
        },
        {
            name: 'python',
            displayName: 'Python',
            scopeName: 'source.python',
            path: `${BASE_PATH}/python.json`,
        },
        {
            name: 'java',
            displayName: 'Java',
            scopeName: 'source.java',
            path: `${BASE_PATH}/java.json`,
        },
        {
            name: 'cpp',
            displayName: 'C++',
            scopeName: 'source.cpp',
            path: `${BASE_PATH}/cpp.json`,
        },
        {
            name: 'c',
            displayName: 'C',
            scopeName: 'source.c',
            path: `${BASE_PATH}/c.json`,
        },
        {
            name: 'csharp',
            displayName: 'C#',
            scopeName: 'source.cs',
            path: `${BASE_PATH}/csharp.json`,
        },
        {
            name: 'rust',
            displayName: 'Rust',
            scopeName: 'source.rust',
            path: `${BASE_PATH}/rust.json`,
        },
        {
            name: 'go',
            displayName: 'Go',
            scopeName: 'source.go',
            path: `${BASE_PATH}/go.json`,
        },
        {
            name: 'ruby',
            displayName: 'Ruby',
            scopeName: 'source.ruby',
            path: `${BASE_PATH}/ruby.json`,
        },
        {
            name: 'php',
            displayName: 'PHP',
            scopeName: 'source.php',
            path: `${BASE_PATH}/php.json`,
        },
        {
            name: 'html',
            displayName: 'HTML',
            scopeName: 'text.html.basic',
            path: `${BASE_PATH}/html.json`,
        },
        {
            name: 'css',
            displayName: 'CSS',
            scopeName: 'source.css',
            path: `${BASE_PATH}/css.json`,
        },
        {
            name: 'json',
            displayName: 'JSON',
            scopeName: 'source.json',
            path: `${BASE_PATH}/json.json`,
        },
        {
            name: 'yaml',
            displayName: 'YAML',
            scopeName: 'source.yaml',
            path: `${BASE_PATH}/yaml.json`,
        },
        {
            name: 'xml',
            displayName: 'XML',
            scopeName: 'text.xml',
            path: `${BASE_PATH}/xml.json`,
        },
        {
            name: 'markdown',
            displayName: 'Markdown',
            scopeName: 'text.html.markdown',
            path: `${BASE_PATH}/markdown.json`,
        },
        {
            name: 'bash',
            displayName: 'Bash',
            scopeName: 'source.shell',
            path: `${BASE_PATH}/shellscript.json`,
        },
        {
            name: 'sql',
            displayName: 'SQL',
            scopeName: 'source.sql',
            path: `${BASE_PATH}/sql.json`,
        },
        {
            name: 'swift',
            displayName: 'Swift',
            scopeName: 'source.swift',
            path: `${BASE_PATH}/swift.json`,
        },
        {
            name: 'kotlin',
            displayName: 'Kotlin',
            scopeName: 'source.kotlin',
            path: `${BASE_PATH}/kotlin.json`,
        },
        {
            name: 'scala',
            displayName: 'Scala',
            scopeName: 'source.scala',
            path: `${BASE_PATH}/scala.json`,
        },
        {
            name: 'haskell',
            displayName: 'Haskell',
            scopeName: 'source.haskell',
            path: `${BASE_PATH}/haskell.json`,
        },
        {
            name: 'lua',
            displayName: 'Lua',
            scopeName: 'source.lua',
            path: `${BASE_PATH}/lua.json`,
        },
        {
            name: 'r',
            displayName: 'R',
            scopeName: 'source.r',
            path: `${BASE_PATH}/r.json`,
        },
        {
            name: 'jsx',
            displayName: 'JSX',
            scopeName: 'source.js.jsx',
            path: `${BASE_PATH}/javascript.json`,
        },
        {
            name: 'tsx',
            displayName: 'TSX',
            scopeName: 'source.tsx',
            path: `${BASE_PATH}/tsx.json`,
        },
        {
            name: 'vue',
            displayName: 'Vue',
            scopeName: 'source.vue',
            path: `${BASE_PATH}/vue.json`,
        },
        {
            name: 'dockerfile',
            displayName: 'Dockerfile',
            scopeName: 'source.dockerfile',
            path: `${BASE_PATH}/docker.json`,
        },
        {
            name: 'graphql',
            displayName: 'GraphQL',
            scopeName: 'source.graphql',
            path: `${BASE_PATH}/graphql.json`,
        },
    ];
}
