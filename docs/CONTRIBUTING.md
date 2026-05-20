# Contributing to head_click

Obrigado pelo interesse em contribuir com o projeto head_click. Esta documentação descreve o fluxo de contribuição e as diretrizes de qualidade.

## Como contribuir

1. Faça um fork do repositório no GitHub.
2. Clone seu fork localmente.
3. Crie uma branch com nome claro e descritivo:
   - `feature/<descrição>`
   - `fix/<descrição>`
   - `docs/<descrição>`
4. Faça alterações pequenas e revisáveis.
5. Teste localmente antes de abrir um pull request.
6. Abra um pull request para `master` com descrição clara.

## Estilo de commits

Use mensagens de commit claras e atômicas. Exemplo de padrão recomendado:

- `feat:` - nova funcionalidade
- `fix:` - correção de bug
- `docs:` - documentação
- `chore:` - tarefa de manutenção
- `refactor:` - refatoração de código

Exemplo:

```sh
git commit -m "feat: add ESP-NOW receiver stub"
```

## Padrões de código

- Código em C deve seguir estilo consistente com o ESP-IDF.
- Use nomes claros e descritivos para variáveis, funções e arquivos.
- Separe domínio, transporte e HID em componentes distintos.
- Comente o código de forma técnica, objetiva e no estilo Linux.

## Estrutura do repositório

- `main/` - ponto de entrada do firmware.
- `components/` - componentes separados por responsabilidade.
- `docs/` - documentação de arquitetura e contribuição.
- `LICENSE` - licença Apache 2.0.

## Testes e build

Execute o build antes de submeter alterações:

```sh
idf.py -B /mnt/rambuild/head_click build
```

## Pull Request

Ao abrir um PR, inclua:

- Objetivo da mudança.
- Comportamento atual e esperado.
- Instruções de build/teste.
- Referência a issues, se houver.

## Licença

Este repositório está licenciado sob a Apache License 2.0. Ao contribuir, você concorda em submeter seu trabalho sob os termos dessa licença.
