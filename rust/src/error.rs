use std::fmt;

#[derive(Debug)]
pub enum CsonpathError {
    CompileError(String),
    NullResult,
}

impl fmt::Display for CsonpathError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            CsonpathError::CompileError(msg) => write!(f, "compile error: {msg}"),
            CsonpathError::NullResult => write!(f, "null result"),
        }
    }
}

impl std::error::Error for CsonpathError {}
