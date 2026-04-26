#!/usr/bin/env node

/**
 * Test LLM Providers
 * Usage: node scripts/test-providers.js [opencode|ollama]
 */

const { execSync } = require('child_process');

const provider = process.argv[2] || 'opencode';

async function testOpenCode() {
  console.log('Testing OpenCode CLI Provider...\n');
  
  try {
    console.log('$ opencode run "What is 2+2? Answer with just the number." -m github-copilot/claude-haiku-4.5\n');
    
    const result = execSync(
      'opencode run "What is 2+2? Answer with just the number." -m github-copilot/claude-haiku-4.5',
      { encoding: 'utf-8', timeout: 60000 }
    );
    
    console.log('Output:');
    console.log(result);
    console.log('\n✅ OpenCode CLI works!');
  } catch (error) {
    console.error('❌ OpenCode CLI failed:', error.message);
  }
}

async function testOllama() {
  console.log('Testing Ollama Provider...\n');
  
  try {
    const result = execSync(
      'curl -s http://localhost:11434/api/generate -d \'{"model":"llama3.2-vision","prompt":"What is 2+2? Answer with just the number.","stream":false}\'',
      { encoding: 'utf-8', timeout: 30000 }
    );
    
    const data = JSON.parse(result);
    console.log('Response:', data.response);
    console.log('\n✅ Ollama works!');
  } catch (error) {
    console.error('❌ Ollama failed:', error.message);
  }
}

async function testEmbedding() {
  console.log('\nTesting Embeddings (Ollama)...\n');
  
  try {
    const result = execSync(
      'curl -s http://localhost:11434/api/embeddings -d \'{"model":"nomic-embed-text","prompt":"test text"}\'',
      { encoding: 'utf-8', timeout: 30000 }
    );
    
    const data = JSON.parse(result);
    console.log('Embedding dimension:', data.embedding?.length || 0);
    console.log('\n✅ Embeddings work!');
  } catch (error) {
    console.error('❌ Embeddings failed:', error.message);
    console.log('(This is OK if nomic-embed-text is not installed)');
  }
}

if (provider === 'ollama') {
  testOllama();
  testEmbedding();
} else {
  testOpenCode();
}
