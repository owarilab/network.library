/**
 * ConditionalAction ユニットテスト
 * 
 * テスト対象:
 * - evaluateCondition()
 * - _evaluateCompare()
 * - _evaluateEquals()
 * - _evaluateTruthy()
 * - _evaluateHas()
 * - _evaluateExists()
 * - _resolveValue()
 */

// Mock AppData for testing
class MockAppData {
  constructor() {
    this.variables = {
      user: {
        persistent: {
          hp: 50,
          score: 100,
          level: 3,
          state: 'defending',
          hasShield: true,
          inventory: { potion: 5, antidote: 2 },
          questStatus: 'active'
        },
        fixed: {
          playerName: 'Hero'
        }
      },
      system: {
        fixed: {
          isPaused: false
        }
      }
    };
  }

  getRuntimeGlobalVariable(path) {
    const parts = path.split('.');
    let current = this.variables;
    for (const part of parts) {
      if (current && typeof current === 'object') {
        current = current[part];
      } else {
        return undefined;
      }
    }
    return current;
  }

  hasRuntimeGlobalVariable(path) {
    return this.getRuntimeGlobalVariable(path) !== undefined;
  }
}

// Test Suite
class ConditionalActionTest {
  static runAllTests() {
    console.log('=== ConditionalAction Unit Tests ===\n');
    
    const results = [];
    results.push(this.testCompareCondition());
    results.push(this.testEqualsCondition());
    results.push(this.testTruthyCondition());
    results.push(this.testHasCondition());
    results.push(this.testExistsCondition());
    results.push(this.testResolveValue());
    results.push(this.testNullCondition());
    
    console.log('\n=== Summary ===');
    const passed = results.filter(r => r).length;
    const total = results.length;
    console.log(`Passed: ${passed}/${total}`);
    
    return passed === total;
  }

  static testCompareCondition() {
    console.log('### Test: Compare Condition');
    const appData = new MockAppData();
    let passed = 0;
    let total = 0;

    // Test: >
    total++;
    if (ConditionalAction.evaluateCondition({
      type: 'compare',
      left: '${user.persistent.hp}',
      operator: '>',
      right: 30
    }, appData) === true) {
      console.log('✓ compare > works');
      passed++;
    } else {
      console.log('✗ compare > failed');
    }

    // Test: >=
    total++;
    if (ConditionalAction.evaluateCondition({
      type: 'compare',
      left: '${user.persistent.hp}',
      operator: '>=',
      right: 50
    }, appData) === true) {
      console.log('✓ compare >= works');
      passed++;
    } else {
      console.log('✗ compare >= failed');
    }

    // Test: <
    total++;
    if (ConditionalAction.evaluateCondition({
      type: 'compare',
      left: '${user.persistent.level}',
      operator: '<',
      right: 5
    }, appData) === true) {
      console.log('✓ compare < works');
      passed++;
    } else {
      console.log('✗ compare < failed');
    }

    // Test: <=
    total++;
    if (ConditionalAction.evaluateCondition({
      type: 'compare',
      left: '${user.persistent.level}',
      operator: '<=',
      right: 3
    }, appData) === true) {
      console.log('✓ compare <= works');
      passed++;
    } else {
      console.log('✗ compare <= failed');
    }

    // Test: ===
    total++;
    if (ConditionalAction.evaluateCondition({
      type: 'compare',
      left: '${user.persistent.score}',
      operator: '===',
      right: 100
    }, appData) === true) {
      console.log('✓ compare === works');
      passed++;
    } else {
      console.log('✗ compare === failed');
    }

    // Test: !==
    total++;
    if (ConditionalAction.evaluateCondition({
      type: 'compare',
      left: '${user.persistent.hp}',
      operator: '!==',
      right: 100
    }, appData) === true) {
      console.log('✓ compare !== works');
      passed++;
    } else {
      console.log('✗ compare !== failed');
    }

    // Test: literal values
    total++;
    if (ConditionalAction.evaluateCondition({
      type: 'compare',
      left: 50,
      operator: '>=',
      right: 30
    }, appData) === true) {
      console.log('✓ compare with literals works');
      passed++;
    } else {
      console.log('✗ compare with literals failed');
    }

    // Test: NaN handling
    total++;
    if (ConditionalAction.evaluateCondition({
      type: 'compare',
      left: 'invalid',
      operator: '>',
      right: 10
    }, appData) === false) {
      console.log('✓ NaN handling works');
      passed++;
    } else {
      console.log('✗ NaN handling failed');
    }

    console.log(`  Result: ${passed}/${total} passed\n`);
    return passed === total;
  }

  static testEqualsCondition() {
    console.log('### Test: Equals Condition');
    const appData = new MockAppData();
    let passed = 0;
    let total = 0;

    // Test: string equality
    total++;
    if (ConditionalAction.evaluateCondition({
      type: 'equals',
      left: '${user.persistent.state}',
      right: 'defending'
    }, appData) === true) {
      console.log('✓ equals string works');
      passed++;
    } else {
      console.log('✗ equals string failed');
    }

    // Test: mismatch
    total++;
    if (ConditionalAction.evaluateCondition({
      type: 'equals',
      left: '${user.persistent.state}',
      right: 'attacking'
    }, appData) === false) {
      console.log('✓ equals mismatch works');
      passed++;
    } else {
      console.log('✗ equals mismatch failed');
    }

    // Test: case sensitive
    total++;
    if (ConditionalAction.evaluateCondition({
      type: 'equals',
      left: '${user.persistent.state}',
      right: 'Defending'
    }, appData) === false) {
      console.log('✓ equals case-sensitive works');
      passed++;
    } else {
      console.log('✗ equals case-sensitive failed');
    }

    // Test: literal values
    total++;
    if (ConditionalAction.evaluateCondition({
      type: 'equals',
      left: 'test',
      right: 'test'
    }, appData) === true) {
      console.log('✓ equals with literals works');
      passed++;
    } else {
      console.log('✗ equals with literals failed');
    }

    console.log(`  Result: ${passed}/${total} passed\n`);
    return passed === total;
  }

  static testTruthyCondition() {
    console.log('### Test: Truthy Condition');
    const appData = new MockAppData();
    let passed = 0;
    let total = 0;

    // Test: true value
    total++;
    if (ConditionalAction.evaluateCondition({
      type: 'truthy',
      left: '${user.persistent.hasShield}'
    }, appData) === true) {
      console.log('✓ truthy true value works');
      passed++;
    } else {
      console.log('✗ truthy true value failed');
    }

    // Test: numeric non-zero
    total++;
    if (ConditionalAction.evaluateCondition({
      type: 'truthy',
      left: '${user.persistent.level}'
    }, appData) === true) {
      console.log('✓ truthy numeric non-zero works');
      passed++;
    } else {
      console.log('✗ truthy numeric non-zero failed');
    }

    // Test: zero is falsy
    total++;
    if (ConditionalAction.evaluateCondition({
      type: 'truthy',
      left: 0
    }, appData) === false) {
      console.log('✓ truthy zero is falsy works');
      passed++;
    } else {
      console.log('✗ truthy zero is falsy failed');
    }

    // Test: empty string is falsy
    total++;
    if (ConditionalAction.evaluateCondition({
      type: 'truthy',
      left: ''
    }, appData) === false) {
      console.log('✓ truthy empty string is falsy works');
      passed++;
    } else {
      console.log('✗ truthy empty string is falsy failed');
    }

    console.log(`  Result: ${passed}/${total} passed\n`);
    return passed === total;
  }

  static testHasCondition() {
    console.log('### Test: Has Condition');
    const appData = new MockAppData();
    let passed = 0;
    let total = 0;

    // Test: property exists
    total++;
    if (ConditionalAction.evaluateCondition({
      type: 'has',
      left: '${user.persistent.inventory}',
      right: 'potion'
    }, appData) === true) {
      console.log('✓ has existing property works');
      passed++;
    } else {
      console.log('✗ has existing property failed');
    }

    // Test: property not exists
    total++;
    if (ConditionalAction.evaluateCondition({
      type: 'has',
      left: '${user.persistent.inventory}',
      right: 'sword'
    }, appData) === false) {
      console.log('✓ has non-existing property works');
      passed++;
    } else {
      console.log('✗ has non-existing property failed');
    }

    // Test: non-object left value
    total++;
    if (ConditionalAction.evaluateCondition({
      type: 'has',
      left: '${user.persistent.hp}',
      right: 'value'
    }, appData) === false) {
      console.log('✓ has non-object handling works');
      passed++;
    } else {
      console.log('✗ has non-object handling failed');
    }

    console.log(`  Result: ${passed}/${total} passed\n`);
    return passed === total;
  }

  static testExistsCondition() {
    console.log('### Test: Exists Condition');
    const appData = new MockAppData();
    let passed = 0;
    let total = 0;

    // Test: variable exists
    total++;
    if (ConditionalAction.evaluateCondition({
      type: 'exists',
      left: 'user.persistent.hp'
    }, appData) === true) {
      console.log('✓ exists existing variable works');
      passed++;
    } else {
      console.log('✗ exists existing variable failed');
    }

    // Test: variable not exists
    total++;
    if (ConditionalAction.evaluateCondition({
      type: 'exists',
      left: 'user.persistent.nonexistent'
    }, appData) === false) {
      console.log('✓ exists non-existing variable works');
      passed++;
    } else {
      console.log('✗ exists non-existing variable failed');
    }

    console.log(`  Result: ${passed}/${total} passed\n`);
    return passed === total;
  }

  static testResolveValue() {
    console.log('### Test: _resolveValue');
    const appData = new MockAppData();
    let passed = 0;
    let total = 0;

    // Test: template variable
    total++;
    const resolved1 = ConditionalAction._resolveValue('${user.persistent.hp}', appData);
    if (resolved1 === 50) {
      console.log('✓ resolve template variable works');
      passed++;
    } else {
      console.log(`✗ resolve template variable failed (got ${resolved1})`);
    }

    // Test: literal number
    total++;
    const resolved2 = ConditionalAction._resolveValue(100, appData);
    if (resolved2 === 100) {
      console.log('✓ resolve literal number works');
      passed++;
    } else {
      console.log(`✗ resolve literal number failed (got ${resolved2})`);
    }

    // Test: literal string
    total++;
    const resolved3 = ConditionalAction._resolveValue('test', appData);
    if (resolved3 === 'test') {
      console.log('✓ resolve literal string works');
      passed++;
    } else {
      console.log(`✗ resolve literal string failed (got ${resolved3})`);
    }

    // Test: undefined variable
    total++;
    const resolved4 = ConditionalAction._resolveValue('${user.persistent.nonexistent}', appData);
    if (resolved4 === undefined) {
      console.log('✓ resolve undefined variable works');
      passed++;
    } else {
      console.log(`✗ resolve undefined variable failed (got ${resolved4})`);
    }

    // Test: template path with whitespace
    total++;
    const resolved5 = ConditionalAction._resolveValue('${ user.persistent.hp }', appData);
    if (resolved5 === 50) {
      console.log('✓ resolve template path with whitespace works');
      passed++;
    } else {
      console.log(`✗ resolve template path with whitespace failed (got ${resolved5})`);
    }

    console.log(`  Result: ${passed}/${total} passed\n`);
    return passed === total;
  }

  static testNullCondition() {
    console.log('### Test: Null/Undefined Condition');
    const appData = new MockAppData();
    let passed = 0;
    let total = 0;

    // Test: null condition
    total++;
    if (ConditionalAction.evaluateCondition(null, appData) === false) {
      console.log('✓ null condition is rejected');
      passed++;
    } else {
      console.log('✗ null condition failed');
    }

    // Test: undefined condition
    total++;
    if (ConditionalAction.evaluateCondition(undefined, appData) === false) {
      console.log('✓ undefined condition is rejected');
      passed++;
    } else {
      console.log('✗ undefined condition failed');
    }

    console.log(`  Result: ${passed}/${total} passed\n`);
    return passed === total;
  }
}

// Run tests when this script is loaded
if (typeof ConditionalAction !== 'undefined') {
  console.log('ConditionalAction loaded. Running tests...\n');
  ConditionalActionTest.runAllTests();
} else {
  console.error('ConditionalAction class not found. Make sure conditional_action.js is loaded first.');
}
