use crate::error::LoxError;
use crate::token::{Token, TokenKind};

pub struct Scanner<'a> {
    _source: &'a str,
    chars: std::iter::Peekable<std::str::Chars<'a>>,
    line: usize,
}

impl<'a> Scanner<'a> {
    pub fn new(source: &'a str) -> Self {
        Self {
            _source: source,
            chars: source.chars().peekable(),
            line: 1,
        }
    }

    pub fn scan_tokens(mut self) -> Result<Vec<Token>, LoxError> {
        let mut tokens = Vec::new();
        while !self.is_at_end() {
            self.skip_whitespace();
            if self.is_at_end() {
                break;
            }
            let start_line = self.line;
            let c = self.advance();
            match c {
                '(' => tokens.push(simple(TokenKind::LeftParen, "(", start_line)),
                ')' => tokens.push(simple(TokenKind::RightParen, ")", start_line)),
                '{' => tokens.push(simple(TokenKind::LeftBrace, "{", start_line)),
                '}' => tokens.push(simple(TokenKind::RightBrace, "}", start_line)),
                ',' => tokens.push(simple(TokenKind::Comma, ",", start_line)),
                '.' => tokens.push(simple(TokenKind::Dot, ".", start_line)),
                '-' => tokens.push(simple(TokenKind::Minus, "-", start_line)),
                '+' => tokens.push(simple(TokenKind::Plus, "+", start_line)),
                ';' => tokens.push(simple(TokenKind::Semicolon, ";", start_line)),
                '*' => tokens.push(simple(TokenKind::Star, "*", start_line)),
                '!' => {
                    let kind = if self.match_char('=') {
                        TokenKind::BangEqual
                    } else {
                        TokenKind::Bang
                    };
                    tokens.push(simple(kind, "", start_line));
                }
                '=' => {
                    let kind = if self.match_char('=') {
                        TokenKind::EqualEqual
                    } else {
                        TokenKind::Equal
                    };
                    tokens.push(simple(kind, "", start_line));
                }
                '<' => {
                    let kind = if self.match_char('=') {
                        TokenKind::LessEqual
                    } else {
                        TokenKind::Less
                    };
                    tokens.push(simple(kind, "", start_line));
                }
                '>' => {
                    let kind = if self.match_char('=') {
                        TokenKind::GreaterEqual
                    } else {
                        TokenKind::Greater
                    };
                    tokens.push(simple(kind, "", start_line));
                }
                '/' => {
                    if self.match_char('/') {
                        while self.peek() != '\n' && !self.is_at_end() {
                            self.advance();
                        }
                    } else {
                        tokens.push(simple(TokenKind::Slash, "/", start_line));
                    }
                }
                '"' => tokens.push(self.string(start_line)?),
                c if c.is_ascii_digit() => tokens.push(self.number(c, start_line)),
                c if is_ident_start(c) => tokens.push(self.identifier(c, start_line)),
                '\n' => self.line += 1,
                _ => return Err(LoxError::new(start_line, format!("unexpected character '{c}'"))),
            }
        }
        tokens.push(Token {
            kind: TokenKind::Eof,
            lexeme: String::new(),
            line: self.line,
        });
        Ok(tokens)
    }

    fn string(&mut self, line: usize) -> Result<Token, LoxError> {
        let mut value = String::new();
        while self.peek() != '"' && !self.is_at_end() {
            if self.peek() == '\n' {
                self.line += 1;
            }
            value.push(self.advance());
        }
        if self.is_at_end() {
            return Err(LoxError::new(line, "unterminated string"));
        }
        self.advance(); // closing "
        Ok(Token {
            kind: TokenKind::String,
            lexeme: value,
            line,
        })
    }

    fn number(&mut self, first: char, line: usize) -> Token {
        let mut lexeme = String::from(first);
        while self.peek().is_ascii_digit() {
            lexeme.push(self.advance());
        }
        if self.peek() == '.' && self.peek_next().is_ascii_digit() {
            lexeme.push(self.advance());
            while self.peek().is_ascii_digit() {
                lexeme.push(self.advance());
            }
        }
        Token {
            kind: TokenKind::Number,
            lexeme,
            line,
        }
    }

    fn identifier(&mut self, first: char, line: usize) -> Token {
        let mut lexeme = String::from(first);
        while is_ident_part(self.peek()) {
            lexeme.push(self.advance());
        }
        let kind = match lexeme.as_str() {
            "and" => TokenKind::And,
            "class" => TokenKind::Class,
            "else" => TokenKind::Else,
            "false" => TokenKind::False,
            "for" => TokenKind::For,
            "fun" => TokenKind::Fun,
            "if" => TokenKind::If,
            "nil" => TokenKind::Nil,
            "or" => TokenKind::Or,
            "print" => TokenKind::Print,
            "return" => TokenKind::Return,
            "super" => TokenKind::Super,
            "this" => TokenKind::This,
            "true" => TokenKind::True,
            "var" => TokenKind::Var,
            "while" => TokenKind::While,
            _ => TokenKind::Identifier,
        };
        Token { kind, lexeme, line }
    }

    fn skip_whitespace(&mut self) {
        loop {
            match self.peek() {
                ' ' | '\r' | '\t' => {
                    self.advance();
                }
                '\n' => {
                    self.line += 1;
                    self.advance();
                }
                _ => break,
            }
        }
    }

    fn is_at_end(&mut self) -> bool {
        self.peek() == '\0'
    }

    fn peek(&mut self) -> char {
        self.chars.peek().copied().unwrap_or('\0')
    }

    fn peek_next(&mut self) -> char {
        let mut clone = self.chars.clone();
        clone.next();
        clone.peek().copied().unwrap_or('\0')
    }

    fn advance(&mut self) -> char {
        self.chars.next().unwrap_or('\0')
    }

    fn match_char(&mut self, expected: char) -> bool {
        if self.peek() == expected {
            self.advance();
            true
        } else {
            false
        }
    }
}

fn simple(kind: TokenKind, lexeme: &str, line: usize) -> Token {
    Token {
        kind,
        lexeme: lexeme.to_string(),
        line,
    }
}

fn is_ident_start(c: char) -> bool {
    c.is_ascii_alphabetic() || c == '_'
}

fn is_ident_part(c: char) -> bool {
    c.is_ascii_alphanumeric() || c == '_'
}
